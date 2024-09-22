# frozen_string_literal: true

require 'bundler/gem_tasks'
require 'rspec/core/rake_task'
require 'rake/extensiontask'

RSpec::Core::RakeTask.new(:spec)

Rake::ExtensionTask.new('wow_dbc') do |ext|
  ext.lib_dir = 'lib/wow_dbc'
end

task default: [:compile, :spec]
