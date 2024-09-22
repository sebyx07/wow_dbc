#include <ruby.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

typedef enum {
    TYPE_UINT32,
    TYPE_INT32,
    TYPE_FLOAT,
    TYPE_STRING
} FieldType;

typedef struct {
    FieldType type;
    union {
        uint32_t uint32_value;
        int32_t int32_value;
        float float_value;
        uint32_t string_offset;
    } value;
} FieldValue;

typedef struct {
    char magic[4];
    uint32_t record_count;
    uint32_t field_count;
    uint32_t record_size;
    uint32_t string_block_size;
} DBCHeader;

typedef struct {
    DBCHeader header;
    FieldValue **records;
    char *string_block;
    VALUE field_definitions;  // Ruby hash of field names and types
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
           (dbc->header.record_count * sizeof(FieldValue *)) +
           (dbc->header.record_count * dbc->header.field_count * sizeof(FieldValue)) +
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

static FieldType ruby_to_field_type(VALUE type_value) {
    ID type_id;

    if (RB_TYPE_P(type_value, T_SYMBOL)) {
        type_id = SYM2ID(type_value);
    } else if (RB_TYPE_P(type_value, T_STRING)) {
        type_id = rb_intern(StringValueCStr(type_value));
    } else {
        rb_raise(rb_eTypeError, "Field type must be a symbol or string");
    }

    if (type_id == rb_intern("uint32")) return TYPE_UINT32;
    if (type_id == rb_intern("int32")) return TYPE_INT32;
    if (type_id == rb_intern("float")) return TYPE_FLOAT;
    if (type_id == rb_intern("string")) return TYPE_STRING;
    rb_raise(rb_eArgError, "Invalid field type");
}

static VALUE dbc_initialize(VALUE self, VALUE filepath, VALUE field_definitions) {
    DBCFile *dbc;
    TypedData_Get_Struct(self, DBCFile, &dbc_data_type, dbc);

    rb_iv_set(self, "@filepath", filepath);
    dbc->field_definitions = rb_hash_dup(field_definitions);
    rb_iv_set(self, "@field_definitions", dbc->field_definitions);

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

    if (dbc->records) {
        for (uint32_t i = 0; i < dbc->header.record_count; i++) {
            free(dbc->records[i]);
        }
        free(dbc->records);
    }

    dbc->records = ALLOC_N(FieldValue *, dbc->header.record_count);
    VALUE field_names = rb_funcall(dbc->field_definitions, rb_intern("keys"), 0);

    for (uint32_t i = 0; i < dbc->header.record_count; i++) {
        dbc->records[i] = ALLOC_N(FieldValue, dbc->header.field_count);
        for (uint32_t j = 0; j < dbc->header.field_count; j++) {
            VALUE field_name = rb_ary_entry(field_names, j);
            VALUE field_type = rb_hash_aref(dbc->field_definitions, field_name);
            FieldType type = ruby_to_field_type(field_type);
            dbc->records[i][j].type = type;

            if (fread(&dbc->records[i][j].value, sizeof(uint32_t), 1, file) != 1) {
                fclose(file);
                rb_raise(rb_eIOError, "Failed to read DBC record field");
            }
        }
    }

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
        for (uint32_t j = 0; j < dbc->header.field_count; j++) {
            if (fwrite(&dbc->records[i][j].value, sizeof(uint32_t), 1, file) != 1) {
                fclose(file);
                rb_raise(rb_eIOError, "Failed to write DBC record field");
            }
        }
    }

    if (fwrite(dbc->string_block, 1, dbc->header.string_block_size, file) != dbc->header.string_block_size) {
        fclose(file);
        rb_raise(rb_eIOError, "Failed to write DBC string block");
    }

    fclose(file);
    return self;
}

static VALUE field_value_to_ruby(FieldValue *value, char *string_block) {
    switch (value->type) {
        case TYPE_UINT32:
            return UINT2NUM(value->value.uint32_value);
        case TYPE_INT32:
            return INT2NUM(value->value.int32_value);
        case TYPE_FLOAT:
            return DBL2NUM(value->value.float_value);
        case TYPE_STRING:
            return rb_str_new2(&string_block[value->value.string_offset]);
    }
    return Qnil;
}

static void ruby_to_field_value(VALUE ruby_value, FieldType type, FieldValue *field_value) {
    field_value->type = type;
    switch (type) {
        case TYPE_UINT32:
            field_value->value.uint32_value = NUM2UINT(ruby_value);
            break;
        case TYPE_INT32:
            field_value->value.int32_value = NUM2INT(ruby_value);
            break;
        case TYPE_FLOAT:
            field_value->value.float_value = (float)NUM2DBL(ruby_value);
            break;
        case TYPE_STRING:
            field_value->value.string_offset = NUM2UINT(ruby_value);
            break;
    }
}

static VALUE dbc_create_record(VALUE self) {
    DBCFile *dbc;
    TypedData_Get_Struct(self, DBCFile, &dbc_data_type, dbc);

    uint32_t new_count = dbc->header.record_count + 1;
    REALLOC_N(dbc->records, FieldValue *, new_count);
    dbc->records[new_count - 1] = ALLOC_N(FieldValue, dbc->header.field_count);
    memset(dbc->records[new_count - 1], 0, dbc->header.field_count * sizeof(FieldValue));

    dbc->header.record_count = new_count;

    return INT2FIX(new_count - 1);
}

static VALUE dbc_update_record(VALUE self, VALUE index, VALUE field_name, VALUE value) {
    DBCFile *dbc;
    TypedData_Get_Struct(self, DBCFile, &dbc_data_type, dbc);

    long idx = FIX2LONG(index);
    long field_idx = -1;

    VALUE field_names = rb_funcall(dbc->field_definitions, rb_intern("keys"), 0);
    for (long i = 0; i < RARRAY_LEN(field_names); i++) {
        if (rb_eql(rb_ary_entry(field_names, i), field_name)) {
            field_idx = i;
            break;
        }
    }

    if (idx < 0 || (uint32_t)idx >= dbc->header.record_count || field_idx < 0 || (uint32_t)field_idx >= dbc->header.field_count) {
        rb_raise(rb_eArgError, "Invalid record or field index");
    }

    VALUE field_type = rb_hash_aref(dbc->field_definitions, field_name);
    FieldType type = ruby_to_field_type(field_type);
    ruby_to_field_value(value, type, &dbc->records[idx][field_idx]);

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
    VALUE field_names = rb_funcall(dbc->field_definitions, rb_intern("keys"), 0);
    for (uint32_t i = 0; i < dbc->header.field_count; i++) {
        VALUE field_name = rb_ary_entry(field_names, i);
        rb_hash_aset(record, field_name, field_value_to_ruby(&dbc->records[idx][i], dbc->string_block));
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

            VALUE field_names = rb_funcall(dbc->field_definitions, rb_intern("keys"), 0);
            for (long j = 0; j < RARRAY_LEN(field_names); j++) {
                if (rb_eql(rb_ary_entry(field_names, j), key)) {
                    field_idx = j;
                    break;
                }
            }

            if (field_idx >= 0 && (uint32_t)field_idx < dbc->header.field_count) {
                VALUE field_type = rb_hash_aref(dbc->field_definitions, key);
                FieldType type = ruby_to_field_type(field_type);
                ruby_to_field_value(value, type, &dbc->records[idx][field_idx]);
            } else {
                rb_raise(rb_eArgError, "Invalid field name: %"PRIsVALUE, key);
            }
        }
    } else {
        rb_raise(rb_eArgError, "Updates must be a hash");
    }

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
    memmove(&dbc->records[idx], &dbc->records[idx + 1], (dbc->header.record_count - idx - 1) * sizeof(FieldValue *));
    dbc->header.record_count--;

    return Qnil;
}

static VALUE dbc_find_by(VALUE self, VALUE field, VALUE value) {
    DBCFile *dbc;
    TypedData_Get_Struct(self, DBCFile, &dbc_data_type, dbc);

    long field_idx = -1;
    VALUE field_names = rb_funcall(dbc->field_definitions, rb_intern("keys"), 0);

    for (long i = 0; i < RARRAY_LEN(field_names); i++) {
        if (rb_eql(rb_ary_entry(field_names, i), field)) {
            field_idx = i;
            break;
        }
    }

    if (field_idx < 0 || (uint32_t)field_idx >= dbc->header.field_count) {
        rb_raise(rb_eArgError, "Invalid field name");
    }

    VALUE result = rb_ary_new();

    for (uint32_t i = 0; i < dbc->header.record_count; i++) {
        VALUE field_value = field_value_to_ruby(&dbc->records[i][field_idx], dbc->string_block);
        if (rb_eql(field_value, value)) {
            VALUE record = rb_hash_new();
            for (uint32_t j = 0; j < dbc->header.field_count; j++) {
                VALUE field_name = rb_ary_entry(field_names, j);
                rb_hash_aset(record, field_name, field_value_to_ruby(&dbc->records[i][j], dbc->string_block));
            }
            rb_ary_push(result, record);
        }
    }

    return result;
}

static VALUE dbc_write_to(VALUE self, VALUE new_filepath) {
    DBCFile *dbc;
    TypedData_Get_Struct(self, DBCFile, &dbc_data_type, dbc);

    Check_Type(new_filepath, T_STRING);
    const char *path = StringValueCStr(new_filepath);

    FILE *file = fopen(path, "wb");
    if (!file) {
        rb_raise(rb_eIOError, "Could not open file for writing: %s", path);
    }

    if (fwrite(&dbc->header, sizeof(DBCHeader), 1, file) != 1) {
        fclose(file);
        rb_raise(rb_eIOError, "Failed to write DBC header");
    }

    for (uint32_t i = 0; i < dbc->header.record_count; i++) {
        for (uint32_t j = 0; j < dbc->header.field_count; j++) {
            if (fwrite(&dbc->records[i][j].value, sizeof(uint32_t), 1, file) != 1) {
                fclose(file);
                rb_raise(rb_eIOError, "Failed to write DBC record");
            }
        }
    }

    if (fwrite(dbc->string_block, 1, dbc->header.string_block_size, file) != dbc->header.string_block_size) {
        fclose(file);
        rb_raise(rb_eIOError, "Failed to write DBC string block");
    }

    fclose(file);
    return self;
}

static VALUE dbc_create_record_with_values(VALUE self, VALUE values) {
    DBCFile *dbc;
    TypedData_Get_Struct(self, DBCFile, &dbc_data_type, dbc);

    if (!RB_TYPE_P(values, T_HASH)) {
        rb_raise(rb_eArgError, "Values must be a hash");
    }

    uint32_t new_count = dbc->header.record_count + 1;
    REALLOC_N(dbc->records, FieldValue *, new_count);
    dbc->records[new_count - 1] = ALLOC_N(FieldValue, dbc->header.field_count);

    VALUE field_names = rb_funcall(dbc->field_definitions, rb_intern("keys"), 0);
    for (uint32_t i = 0; i < dbc->header.field_count; i++) {
        VALUE field_name = rb_ary_entry(field_names, i);
        VALUE value = rb_hash_aref(values, field_name);

        if (NIL_P(value)) {
            rb_raise(rb_eArgError, "Missing value for field: %"PRIsVALUE, field_name);
        }

        VALUE field_type = rb_hash_aref(dbc->field_definitions, field_name);
        FieldType type = ruby_to_field_type(field_type);
        ruby_to_field_value(value, type, &dbc->records[new_count - 1][i]);
    }

    dbc->header.record_count = new_count;

    return INT2FIX(new_count - 1);
}

void Init_wow_dbc(void) {
    rb_mWowDBC = rb_define_module("WowDBC");
    rb_cDBCFile = rb_define_class_under(rb_mWowDBC, "DBCFile", rb_cObject);
    rb_define_alloc_func(rb_cDBCFile, dbc_alloc);
    rb_define_method(rb_cDBCFile, "initialize", dbc_initialize, 2);
    rb_define_method(rb_cDBCFile, "read", dbc_read, 0);
    rb_define_method(rb_cDBCFile, "write", dbc_write, 0);
    rb_define_method(rb_cDBCFile, "write_to", dbc_write_to, 1);
    rb_define_method(rb_cDBCFile, "create_record", dbc_create_record, 0);
    rb_define_method(rb_cDBCFile, "create_record_with_values", dbc_create_record_with_values, 1);
    rb_define_method(rb_cDBCFile, "update_record", dbc_update_record, 3);
    rb_define_method(rb_cDBCFile, "update_record_multi", dbc_update_record_multi, 2);
    rb_define_method(rb_cDBCFile, "delete_record", dbc_delete_record, 1);
    rb_define_method(rb_cDBCFile, "get_record", dbc_get_record, 1);
    rb_define_method(rb_cDBCFile, "header", dbc_get_header, 0);
    rb_define_method(rb_cDBCFile, "find_by", dbc_find_by, 2);
}