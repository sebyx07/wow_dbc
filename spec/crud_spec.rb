# frozen_string_literal: true

# File: spec/wow_dbc/crud_spec.rb

RSpec.describe WowDBC::DBCFile do
  let(:original_file) { File.join(File.dirname(__FILE__), 'resources', 'Item.dbc') }
  let(:test_file) { File.join(File.dirname(__FILE__), 'resources', 'Item_test.dbc') }
  let(:new_file) { File.join(File.dirname(__FILE__), 'resources', 'Item_new.dbc') }
  let(:field_names) { [:id, :class, :subclass, :sound_override_subclass, :material, :displayid, :inventory_type, :sheath_type] }

  before(:each) do
    FileUtils.cp(original_file, test_file)
  end

  after(:each) do
    File.delete(test_file) if File.exist?(test_file)
    File.delete(new_file) if File.exist?(new_file)
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

    it 'creates a new record with initial values' do
      initial_values = { id: 1000, class: 2, subclass: 3 }
      new_record_index = dbc_file.create_record_with_values(initial_values)
      new_record = dbc_file.get_record(new_record_index)

      expect(new_record[:id]).to eq(1000)
      expect(new_record[:class]).to eq(2)
      expect(new_record[:subclass]).to eq(3)
    end

    it 'updates multiple fields of a record at once' do
      original_record = dbc_file.get_record(0)
      updates = { class: 5, subclass: 6, material: 7 }
      dbc_file.update_record_multi(0, updates)
      updated_record = dbc_file.get_record(0)

      expect(updated_record[:class]).to eq(5)
      expect(updated_record[:subclass]).to eq(6)
      expect(updated_record[:material]).to eq(7)
      expect(updated_record[:id]).to eq(original_record[:id])  # Ensure other fields remain unchanged
    end

    it 'creates a record with initial values and then updates multiple fields' do
      initial_values = { id: 2000, class: 3, subclass: 4 }
      new_record_index = dbc_file.create_record_with_values(initial_values)

      updates = { class: 8, material: 9, inventory_type: 10 }
      dbc_file.update_record_multi(new_record_index, updates)

      updated_record = dbc_file.get_record(new_record_index)

      expect(updated_record[:id]).to eq(2000)
      expect(updated_record[:class]).to eq(8)
      expect(updated_record[:subclass]).to eq(4)
      expect(updated_record[:material]).to eq(9)
      expect(updated_record[:inventory_type]).to eq(10)
    end

    it 'finds a record by id' do
      results = dbc_file.find_by(:id, 32837)
      expect(results).to be_an(Array)
      expect(results.length).to eq(1)
      expect(results.first[:id]).to eq(32837)
    end

    it 'returns an empty array when no matching records are found' do
      results = dbc_file.find_by(:id, 999999)  # Assuming this ID doesn't exist
      expect(results).to be_an(Array)
      expect(results).to be_empty
    end

    it 'finds multiple records with the same value in a field' do
      # Assuming there are multiple records with class 2
      results = dbc_file.find_by(:class, 2)
      expect(results).to be_an(Array)
      expect(results.length).to be > 1
      results.each do |record|
        expect(record[:class]).to eq(2)
      end
    end
  end

  describe '#write_to' do
    let(:dbc_file) { WowDBC::DBCFile.new(test_file, field_names) }

    before(:each) do
      dbc_file.read
    end

    it 'writes the DBC file to a new location' do
      dbc_file.write_to(new_file)
      expect(File.exist?(new_file)).to be true
    end

    it 'preserves the original file' do
      original_content = File.read(test_file)
      dbc_file.write_to(new_file)
      expect(File.read(test_file)).to eq(original_content)
    end

    it 'writes a file with the same content as the original' do
      dbc_file.write_to(new_file)
      expect(FileUtils.compare_file(test_file, new_file)).to be true
    end

    it 'writes changes to the new file' do
      new_value = 99999
      dbc_file.update_record(0, :class, new_value)
      dbc_file.write_to(new_file)

      new_dbc_file = WowDBC::DBCFile.new(new_file, field_names)
      new_dbc_file.read
      expect(new_dbc_file.get_record(0)[:class]).to eq(new_value)

      original_dbc_file = WowDBC::DBCFile.new(test_file, field_names)
      original_dbc_file.read
      expect(original_dbc_file.get_record(0)[:class]).not_to eq(new_value)
    end

    it 'raises an error when the new file path is invalid' do
      invalid_path = '/invalid/path/file.dbc'
      expect { dbc_file.write_to(invalid_path) }.to raise_error(IOError)
    end

    it 'overwrites an existing file at the new path' do
      FileUtils.touch(new_file)
      original_content = 'Original content'
      File.write(new_file, original_content)

      dbc_file.write_to(new_file)

      expect(File.read(new_file)).not_to eq(original_content)
      expect(FileUtils.compare_file(test_file, new_file)).to be true
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

    it 'raises an error when trying to create a record with invalid field names' do
      invalid_values = { id: 3000, invalid_field: 5 }
      expect { dbc_file.create_record_with_values(invalid_values) }.to raise_error(ArgumentError)
    end

    it 'raises an error when trying to update multiple fields with invalid field names' do
      invalid_updates = { class: 7, invalid_field: 8 }
      expect { dbc_file.update_record_multi(0, invalid_updates) }.to raise_error(ArgumentError)
    end

    it 'raises an error when trying to update multiple fields of a non-existent record' do
      updates = { class: 9, subclass: 10 }
      expect { dbc_file.update_record_multi(-1, updates) }.to raise_error(ArgumentError)
      expect { dbc_file.update_record_multi(999999, updates) }.to raise_error(ArgumentError)
    end
  end
end
