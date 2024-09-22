# frozen_string_literal: true

# File: spec/wow_dbc/crud_spec.rb

RSpec.describe WowDBC::DBCFile do
  let(:original_file) { File.join(File.dirname(__FILE__), 'resources', 'Item.dbc') }
  let(:test_file) { File.join(File.dirname(__FILE__), 'resources', 'Item_test.dbc') }
  let(:field_names) { [:id, :class, :subclass, :sound_override_subclass, :material, :displayid, :inventory_type, :sheath_type] }

  before(:each) do
    FileUtils.cp(original_file, test_file)
  end

  after(:each) do
    File.delete(test_file) if File.exist?(test_file)
  end

  describe 'CRUD operations' do
    let(:dbc_file) { WowDBC::DBCFile.new(test_file, field_names) }

    before(:each) do
      dbc_file.read
    end

    it 'reads the DBC file correctly' do
      expect(dbc_file).to be_a(WowDBC::DBCFile)
      expect(dbc_file.instance_variable_get(:@filepath)).to eq(test_file)
    end

    it 'creates a new record' do
      initial_count = dbc_file.header[:record_count]
      new_record_index = dbc_file.create_record
      expect(dbc_file.header[:record_count]).to eq(initial_count + 1)
      expect(new_record_index).to eq(initial_count)
    end

    it 'reads a record' do
      record = dbc_file.get_record(0)
      expect(record).to be_a(Hash)
      expect(record.keys).to match_array(field_names)
    end

    it 'updates a record' do
      original_value = dbc_file.get_record(0)[:class]
      new_value = original_value + 1
      dbc_file.update_record(0, :class, new_value)
      updated_record = dbc_file.get_record(0)
      expect(updated_record[:class]).to eq(new_value)
    end

    it 'deletes a record' do
      initial_count = dbc_file.header[:record_count]
      dbc_file.delete_record(initial_count - 1)
      expect(dbc_file.header[:record_count]).to eq(initial_count - 1)
    end

    it 'writes changes to the file' do
      new_value = 99999
      dbc_file.update_record(0, :class, new_value)
      dbc_file.write

      # Read the file again to verify changes
      new_dbc_file = WowDBC::DBCFile.new(test_file, field_names)
      new_dbc_file.read
      expect(new_dbc_file.get_record(0)[:class]).to eq(new_value)
    end
  end

  describe 'error handling' do
    let(:dbc_file) { WowDBC::DBCFile.new(test_file, field_names) }

    before(:each) do
      dbc_file.read
    end

    it 'raises an error when trying to get a non-existent record' do
      expect { dbc_file.get_record(-1) }.to raise_error(ArgumentError)
      expect { dbc_file.get_record(999999) }.to raise_error(ArgumentError)
    end

    it 'raises an error when trying to update a non-existent record' do
      expect { dbc_file.update_record(-1, :id, 0) }.to raise_error(ArgumentError)
      expect { dbc_file.update_record(999999, :id, 0) }.to raise_error(ArgumentError)
    end

    it 'raises an error when trying to update a non-existent field' do
      expect { dbc_file.update_record(0, :non_existent_field, 0) }.to raise_error(ArgumentError)
    end

    it 'raises an error when trying to delete a non-existent record' do
      expect { dbc_file.delete_record(-1) }.to raise_error(ArgumentError)
      expect { dbc_file.delete_record(999999) }.to raise_error(ArgumentError)
    end
  end
end
