# WowDBC 🎮

WowDBC is a high-performance Ruby gem for reading and manipulating World of Warcraft DBC (Database Client) files. 🚀

## Features 🌟

- Fast reading and writing of DBC files
- CRUD operations for DBC records
- Ruby-friendly interface with field name access
- Efficient C extension for optimal performance

## Installation 💎

Add this line to your application's Gemfile:

```ruby
gem 'wow_dbc'
```

And then execute:

```bash
$ bundle install
```

Or install it yourself as:

```bash
$ gem install wow_dbc
```

## Usage 📚

Here's a quick example of how to use WowDBC:

```ruby
require 'wow_dbc'

# Correct field names for the Item.dbc file
field_names = [:id, :class, :subclass, :sound_override_subclass, :material, :displayid, :inventory_type, :sheath_type]

# Open the Item.dbc file
dbc = WowDBC::DBCFile.new('path/to/your/Item.dbc', field_names)
dbc.read

# Find a specific item (e.g., Warglaive of Azzinoth, ID: 32837)
warglaive = dbc.find_by(:id, 32837).first
puts "Warglaive of Azzinoth: #{warglaive}"

# Update a single field of the Warglaive
dbc.update_record(warglaive[:id], :sheath_type, 3)  # Assuming 3 represents a different sheath type

# Update multiple fields of the Warglaive
dbc.update_record_multi(warglaive[:id], { material: 5, inventory_type: 17 })  # Assuming 5 is a different material and 17 is Two-Hand

# Create a new empty item record
new_item_index = dbc.create_record
puts "New empty item index: #{new_item_index}"

# Create a new item record with initial values
initial_values = {
  id: 99999,
  class: 2,  # Weapon
  subclass: 7,  # Warglaives
  sound_override_subclass: -1,  # No override
  material: warglaive[:material],
  displayid: warglaive[:displayid],
  inventory_type: 17,  # Two-Hand
  sheath_type: 3
}
new_item_index = dbc.create_record_with_values(initial_values)
puts "New custom item index: #{new_item_index}"

# Read the newly created item
new_item = dbc.get_record(new_item_index)
puts "Newly created item: #{new_item}"

# Delete an item (be careful with this!)
# dbc.delete_record(new_item_index)

# Write changes back to the file
dbc.write

# Reading header information
header = dbc.header
puts "Total items: #{header[:record_count]}"
puts "Fields per item: #{header[:field_count]}"

# Finding all two-handed weapons
two_handed_weapons = dbc.find_by(:inventory_type, 17)  # 17 represents Two-Hand weapons

puts "Two-handed weapons:"
two_handed_weapons.each do |item|
  puts "Item ID: #{item[:id]}, Class: #{item[:class]}, Subclass: #{item[:subclass]}, Display ID: #{item[:displayid]}"
end
```

## Development 🛠️

After checking out the repo, run `bin/setup` to install dependencies. Then, run `rake spec` to run the tests. You can also run `bin/console` for an interactive prompt that will allow you to experiment.

To install this gem onto your local machine, run `bundle exec rake install`. To release a new version, update the version number in `version.rb`, and then run `bundle exec rake release`, which will create a git tag for the version, push git commits and the created tag, and push the `.gem` file to [rubygems.org](https://rubygems.org).

## Contributing 🤝

Bug reports and pull requests are welcome on GitHub at https://github.com/sebyx07/wow_dbc. This project is intended to be a safe, welcoming space for collaboration, and contributors are expected to adhere to the [code of conduct](https://github.com/sebyx07/wow_dbc/blob/master/CODE_OF_CONDUCT.md).

1. Fork it ( https://github.com/sebyx07/wow_dbc/fork )
2. Create your feature branch (`git checkout -b my-new-feature`)
3. Commit your changes (`git commit -am 'Add some feature'`)
4. Push to the branch (`git push origin my-new-feature`)
5. Create a new Pull Request

## License 📄

The gem is available as open source under the terms of the [MIT License](https://opensource.org/licenses/MIT).

## Code of Conduct 🤝

Everyone interacting in the WowDBC project's codebases, issue trackers, chat rooms and mailing lists is expected to follow the [code of conduct](https://github.com/sebyx07/wow_dbc/blob/master/CODE_OF_CONDUCT.md).

## Acknowledgments 👏

- Thanks to all contributors who have helped shape this project.
- Inspired by the World of Warcraft modding community.

Happy coding, and may your adventures in Azeroth be bug-free! 🐉✨