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

# Define field names for your DBC file
field_names = [:id, :name, :description, :icon, :category, :subcategory]

# Open a DBC file
dbc = WowDBC::DBCFile.new('path/to/your/file.dbc', field_names)
dbc.read

# Read a record
record = dbc.get_record(0)
puts "First record: #{record}"

# Update a single field in a record
dbc.update_record(0, :name, "New Name")

# Update multiple fields in a record
dbc.update_record_multi(0, { name: "Newer Name", category: 5, subcategory: 10 })

# Create a new empty record
new_record_index = dbc.create_record
puts "New empty record index: #{new_record_index}"

# Create a new record with initial values
initial_values = { id: 1000, name: "New Item", category: 3, subcategory: 7 }
new_record_with_values_index = dbc.create_record_with_values(initial_values)
puts "New record with values index: #{new_record_with_values_index}"

# Read the newly created record
new_record = dbc.get_record(new_record_with_values_index)
puts "Newly created record: #{new_record}"

# Delete a record
dbc.delete_record(new_record_index)

# Write changes back to the file
dbc.write

# Reading header information
header = dbc.header
puts "Total records: #{header[:record_count]}"
puts "Fields per record: #{header[:field_count]}"

# Iterating through all records
(0...header[:record_count]).each do |i|
  record = dbc.get_record(i)
  puts "Record #{i}: #{record}"
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