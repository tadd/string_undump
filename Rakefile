require "bundler/gem_tasks"
require 'rake/testtask'
require 'rake/extensiontask'

task :build => :compile

Rake::TestTask.new {|t| t.libs << 'test' }

Rake::ExtensionTask.new("string-undump") do |ext|
  ext.lib_dir = "lib/string-undump"
end

task :default => :test
