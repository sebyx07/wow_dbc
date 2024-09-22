# frozen_string_literal: true

# File: spec/wow_dbc/item_display_info_spec.rb

RSpec.describe WowDBC::DBCFile do
  let(:test_file) { File.join(File.dirname(__FILE__), 'resources', 'ItemDisplayInfo.dbc') }
  let(:field_definitions) do
    {
      id: :uint32,
      model_name_1: :string,
      model_name_2: :string,
      model_texture_1: :string,
      model_texture_2: :string,
      inventory_icon_1: :string,
      inventory_icon_2: :string,
      geoset_group_1: :uint32,
      geoset_group_2: :uint32,
      geoset_group_3: :uint32,
      flags: :uint32,
      spell_visual_id: :uint32,
      group_sound_index: :uint32,
      helmet_geoset_vis_id_1: :uint32,
      helmet_geoset_vis_id_2: :uint32,
      texture_1: :string,
      texture_2: :string,
      texture_3: :string,
      texture_4: :string,
      texture_5: :string,
      texture_6: :string,
      texture_7: :string,
      texture_8: :string,
      item_visual: :uint32,
      particle_color_id: :uint32
    }
  end

  let(:dbc_file) { WowDBC::DBCFile.new(test_file, field_definitions) }

  before(:each) do
    dbc_file.read
  end

  describe 'basic operations' do
    it 'reads the DBC file correctly' do
      expect(dbc_file).to be_a(WowDBC::DBCFile)
      expect(dbc_file.instance_variable_get(:@filepath)).to eq(test_file)
    end

    it 'returns correct header information' do
      header = dbc_file.header
      expect(header[:record_count]).to be > 0
      expect(header[:field_count]).to eq(field_definitions.size)
    end
  end

  describe 'string operations' do
    let(:first_record) { dbc_file.get_record(0) }

    it 'reads string fields correctly' do
      expect(first_record[:model_name_1]).to be_a(String)
      expect(first_record[:model_texture_1]).to be_a(String)
      expect(first_record[:inventory_icon_1]).to be_a(String)
    end

    it 'updates string fields' do
      new_model_name = "NewModelName"
      dbc_file.update_record(0, :model_name_1, new_model_name)
      updated_record = dbc_file.get_record(0)
      expect(updated_record[:model_name_1]).to eq(new_model_name)
    end

    it 'handles empty strings' do
      empty_string_record = dbc_file.create_record_with_values(
        id: 99999,
        model_name_1: "",
        model_name_2: "",
        inventory_icon_1: ""
      )
      record = dbc_file.get_record(empty_string_record)
      expect(record[:model_name_1]).to eq("")
      expect(record[:model_name_2]).to eq("")
      expect(record[:inventory_icon_1]).to eq("")
    end
  end

  describe 'search operations' do
    it 'finds records by string field' do
      sample_model_name = dbc_file.get_record(0)[:model_name_1]
      results = dbc_file.find_by(:model_name_1, sample_model_name)
      expect(results).not_to be_empty
      expect(results.first[:model_name_1]).to eq(sample_model_name)
    end
  end

  describe 'write operations' do
    let(:new_file) { File.join(File.dirname(__FILE__), 'resources', 'ItemDisplayInfo_new.dbc') }

    after(:each) do
      File.delete(new_file) if File.exist?(new_file)
    end

    it 'writes changes to a new file' do
      new_model_name = "NewModelName"
      dbc_file.update_record(0, :model_name_1, new_model_name)
      dbc_file.write_to(new_file)

      new_dbc_file = WowDBC::DBCFile.new(new_file, field_definitions)
      new_dbc_file.read
      expect(new_dbc_file.get_record(0)[:model_name_1]).to eq(new_model_name)
    end
  end
end