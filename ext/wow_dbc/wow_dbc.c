#include <ruby.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

typedef struct {
    char magic[4];
    uint32_t record_count;
    uint32_t field_count;
    uint32_t record_size;
    uint32_t string_block_size;
} DBCHeader;

typedef struct {
    DBCHeader header;
    uint32_t **records;
    char *string_block;
    VALUE field_names; // Ruby array of field names
} DBCFile;

static VALUE rb_mWowDBC;
static VALUE rb_cDBCFile;

static void dbc_free(void *ptr) {
    DBCFile *dbc = (DBCFile *)ptr;
    if (dbc->records) {
        for (uint32_t i = 0; i < dbc->header.record_count; i++) {
            free(dbc->records[i]);
        }
        free(dbc->records);
    }
    if (dbc->string_block) {
        free(dbc->string_block);
    }
    free(dbc);
}

static size_t dbc_memsize(const void *ptr) {
    const DBCFile *dbc = (const DBCFile *)ptr;
    return sizeof(DBCFile) +
           (dbc->header.record_count * sizeof(uint32_t *)) +
           (dbc->header.record_count * dbc->header.field_count * sizeof(uint32_t)) +
           dbc->header.string_block_size;
}

static const rb_data_type_t dbc_data_type = {
    "WowDBC::DBCFile",
    {NULL, dbc_free, dbc_memsize,},
    0, 0,
    RUBY_TYPED_FREE_IMMEDIATELY,
};

static VALUE dbc_alloc(VALUE klass) {
    DBCFile *dbc = ALLOC(DBCFile);
    memset(dbc, 0, sizeof(DBCFile));
    return TypedData_Wrap_Struct(klass, &dbc_data_type, dbc);
}

static VALUE dbc_initialize(VALUE self, VALUE filepath, VALUE field_names) {
    DBCFile *dbc;
    TypedData_Get_Struct(self, DBCFile, &dbc_data_type, dbc);

    rb_iv_set(self, "@filepath", filepath);
    dbc->field_names = rb_ary_dup(field_names);
    rb_iv_set(self, "@field_names", dbc->field_names);

    return self;
}

static VALUE dbc_read(VALUE self) {
    DBCFile *dbc;
    TypedData_Get_Struct(self, DBCFile, &dbc_data_type, dbc);

    VALUE filepath = rb_iv_get(self, "@filepath");
    FILE *file = fopen(StringValueCStr(filepath), "rb");
    if (!file) {
        rb_raise(rb_eIOError, "Could not open file");
    }

    if (fread(&dbc->header, sizeof(DBCHeader), 1, file) != 1) {
        fclose(file);
        rb_raise(rb_eIOError, "Failed to read DBC header");
    }

    // Free existing records if any
    if (dbc->records) {
        for (uint32_t i = 0; i < dbc->header.record_count; i++) {
            free(dbc->records[i]);
        }
        free(dbc->records);
    }

    dbc->records = ALLOC_N(uint32_t *, dbc->header.record_count);
    for (uint32_t i = 0; i < dbc->header.record_count; i++) {
        dbc->records[i] = ALLOC_N(uint32_t, dbc->header.field_count);
        if (fread(dbc->records[i], sizeof(uint32_t), dbc->header.field_count, file) != dbc->header.field_count) {
            fclose(file);
            rb_raise(rb_eIOError, "Failed to read DBC record");
        }
    }

    // Free existing string block if any
    if (dbc->string_block) {
        free(dbc->string_block);
    }

    dbc->string_block = ALLOC_N(char, dbc->header.string_block_size);
    if (fread(dbc->string_block, 1, dbc->header.string_block_size, file) != dbc->header.string_block_size) {
        fclose(file);
        rb_raise(rb_eIOError, "Failed to read DBC string block");
    }

    fclose(file);
    return self;
}

static VALUE dbc_write(VALUE self) {
    DBCFile *dbc;
    TypedData_Get_Struct(self, DBCFile, &dbc_data_type, dbc);

    VALUE filepath = rb_iv_get(self, "@filepath");
    FILE *file = fopen(StringValueCStr(filepath), "wb");
    if (!file) {
        rb_raise(rb_eIOError, "Could not open file for writing");
    }

    if (fwrite(&dbc->header, sizeof(DBCHeader), 1, file) != 1) {
        fclose(file);
        rb_raise(rb_eIOError, "Failed to write DBC header");
    }

    for (uint32_t i = 0; i < dbc->header.record_count; i++) {
        if (fwrite(dbc->records[i], sizeof(uint32_t), dbc->header.field_count, file) != dbc->header.field_count) {
            fclose(file);
            rb_raise(rb_eIOError, "Failed to write DBC record");
        }
    }

    if (fwrite(dbc->string_block, 1, dbc->header.string_block_size, file) != dbc->header.string_block_size) {
        fclose(file);
        rb_raise(rb_eIOError, "Failed to write DBC string block");
    }

    fclose(file);
    return self;
}

static VALUE dbc_create_record(VALUE self) {
    DBCFile *dbc;
    TypedData_Get_Struct(self, DBCFile, &dbc_data_type, dbc);

    uint32_t new_count = dbc->header.record_count + 1;
    REALLOC_N(dbc->records, uint32_t *, new_count);
    dbc->records[new_count - 1] = ALLOC_N(uint32_t, dbc->header.field_count);
    memset(dbc->records[new_count - 1], 0, dbc->header.field_count * sizeof(uint32_t));

    dbc->header.record_count = new_count;

    return INT2FIX(new_count - 1);
}

static VALUE dbc_update_record(VALUE self, VALUE index, VALUE field_name, VALUE value) {
    DBCFile *dbc;
    TypedData_Get_Struct(self, DBCFile, &dbc_data_type, dbc);

    long idx = FIX2LONG(index);
    long field_idx = -1;

    // Find the index of the field name
    for (long i = 0; i < RARRAY_LEN(dbc->field_names); i++) {
        if (rb_eql(rb_ary_entry(dbc->field_names, i), field_name)) {
            field_idx = i;
            break;
        }
    }

    if (idx < 0 || (uint32_t)idx >= dbc->header.record_count || field_idx < 0 || (uint32_t)field_idx >= dbc->header.field_count) {
        rb_raise(rb_eArgError, "Invalid record or field index");
    }

    dbc->records[idx][field_idx] = NUM2UINT(value);

    return Qnil;
}

static VALUE dbc_delete_record(VALUE self, VALUE index) {
    DBCFile *dbc;
    TypedData_Get_Struct(self, DBCFile, &dbc_data_type, dbc);

    long idx = FIX2LONG(index);

    if (idx < 0 || (uint32_t)idx >= dbc->header.record_count) {
        rb_raise(rb_eArgError, "Invalid record index");
    }

    free(dbc->records[idx]);
    memmove(&dbc->records[idx], &dbc->records[idx + 1], (dbc->header.record_count - idx - 1) * sizeof(uint32_t *));
    dbc->header.record_count--;

    return Qnil;
}

static VALUE dbc_get_record(VALUE self, VALUE index) {
    DBCFile *dbc;
    TypedData_Get_Struct(self, DBCFile, &dbc_data_type, dbc);

    long idx = FIX2LONG(index);

    if (idx < 0 || (uint32_t)idx >= dbc->header.record_count) {
        rb_raise(rb_eArgError, "Invalid record index");
    }

    VALUE record = rb_hash_new();
    for (uint32_t i = 0; i < dbc->header.field_count; i++) {
        VALUE field_name = rb_ary_entry(dbc->field_names, i);
        rb_hash_aset(record, field_name, UINT2NUM(dbc->records[idx][i]));
    }

    return record;
}

static VALUE dbc_get_header(VALUE self) {
    DBCFile *dbc;
    TypedData_Get_Struct(self, DBCFile, &dbc_data_type, dbc);

    VALUE header = rb_hash_new();
    rb_hash_aset(header, ID2SYM(rb_intern("magic")), rb_str_new(dbc->header.magic, 4));
    rb_hash_aset(header, ID2SYM(rb_intern("record_count")), UINT2NUM(dbc->header.record_count));
    rb_hash_aset(header, ID2SYM(rb_intern("field_count")), UINT2NUM(dbc->header.field_count));
    rb_hash_aset(header, ID2SYM(rb_intern("record_size")), UINT2NUM(dbc->header.record_size));
    rb_hash_aset(header, ID2SYM(rb_intern("string_block_size")), UINT2NUM(dbc->header.string_block_size));

    return header;
}

static VALUE dbc_create_record_with_values(VALUE self, VALUE initial_values) {
    DBCFile *dbc;
    TypedData_Get_Struct(self, DBCFile, &dbc_data_type, dbc);

    uint32_t new_count = dbc->header.record_count + 1;
    REALLOC_N(dbc->records, uint32_t *, new_count);
    dbc->records[new_count - 1] = ALLOC_N(uint32_t, dbc->header.field_count);
    memset(dbc->records[new_count - 1], 0, dbc->header.field_count * sizeof(uint32_t));

    // Set initial values
    if (RB_TYPE_P(initial_values, T_HASH)) {
        VALUE keys = rb_funcall(initial_values, rb_intern("keys"), 0);
        for (long i = 0; i < RARRAY_LEN(keys); i++) {
            VALUE key = rb_ary_entry(keys, i);
            VALUE value = rb_hash_aref(initial_values, key);
            long field_idx = -1;

            for (long j = 0; j < RARRAY_LEN(dbc->field_names); j++) {
                if (rb_eql(rb_ary_entry(dbc->field_names, j), key)) {
                    field_idx = j;
                    break;
                }
            }

            if (field_idx >= 0 && (uint32_t)field_idx < dbc->header.field_count) {
                dbc->records[new_count - 1][field_idx] = NUM2UINT(value);
            } else {
                // Free the allocated memory before raising the error
                free(dbc->records[new_count - 1]);
                REALLOC_N(dbc->records, uint32_t *, dbc->header.record_count);
                rb_raise(rb_eArgError, "Invalid field name: %s", rb_id2name(SYM2ID(key)));
            }
        }
    }

    dbc->header.record_count = new_count;

    return INT2FIX(new_count - 1);
}

static VALUE dbc_update_record_multi(VALUE self, VALUE index, VALUE updates) {
    DBCFile *dbc;
    TypedData_Get_Struct(self, DBCFile, &dbc_data_type, dbc);

    long idx = FIX2LONG(index);

    if (idx < 0 || (uint32_t)idx >= dbc->header.record_count) {
        rb_raise(rb_eArgError, "Invalid record index");
    }

    if (RB_TYPE_P(updates, T_HASH)) {
        VALUE keys = rb_funcall(updates, rb_intern("keys"), 0);
        for (long i = 0; i < RARRAY_LEN(keys); i++) {
            VALUE key = rb_ary_entry(keys, i);
            VALUE value = rb_hash_aref(updates, key);
            long field_idx = -1;

            for (long j = 0; j < RARRAY_LEN(dbc->field_names); j++) {
                if (rb_eql(rb_ary_entry(dbc->field_names, j), key)) {
                    field_idx = j;
                    break;
                }
            }

            if (field_idx >= 0 && (uint32_t)field_idx < dbc->header.field_count) {
                dbc->records[idx][field_idx] = NUM2UINT(value);
            } else {
                rb_raise(rb_eArgError, "Invalid field name: %s", rb_id2name(SYM2ID(key)));
            }
        }
    } else {
        rb_raise(rb_eArgError, "Updates must be a hash");
    }

    return Qnil;
}

void Init_wow_dbc(void) {
    rb_mWowDBC = rb_define_module("WowDBC");
    rb_cDBCFile = rb_define_class_under(rb_mWowDBC, "DBCFile", rb_cObject);
    rb_define_alloc_func(rb_cDBCFile, dbc_alloc);
    rb_define_method(rb_cDBCFile, "initialize", dbc_initialize, 2);
    rb_define_method(rb_cDBCFile, "read", dbc_read, 0);
    rb_define_method(rb_cDBCFile, "write", dbc_write, 0);
    rb_define_method(rb_cDBCFile, "create_record", dbc_create_record, 0);
    rb_define_method(rb_cDBCFile, "create_record_with_values", dbc_create_record_with_values, 1);
    rb_define_method(rb_cDBCFile, "update_record", dbc_update_record, 3);
    rb_define_method(rb_cDBCFile, "update_record_multi", dbc_update_record_multi, 2);
    rb_define_method(rb_cDBCFile, "delete_record", dbc_delete_record, 1);
    rb_define_method(rb_cDBCFile, "get_record", dbc_get_record, 1);
    rb_define_method(rb_cDBCFile, "header", dbc_get_header, 0);
}