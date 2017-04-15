Gem::Specification.new do |spec|
  spec.name          = "string-undump"
  spec.version       = '0.0.1'
  spec.authors       = ["Tadashi Saito"]
  spec.email         = ["tad.a.digger@gmail.com"]

  spec.summary       = 'String#undump'
  spec.description   = 'Unofficial implementation of String#undump, does inverse of String#dump'
  spec.homepage      = 'https://github.com/tadd/string-undump'

  spec.files         = `git ls-files -z`.split("\x0").reject do |f|
    f.match(%r{^(test|spec|features)/})
  end
  spec.require_paths = ["lib"]

  spec.add_development_dependency "bundler"
  spec.add_development_dependency "rake"
  spec.add_development_dependency "test-unit"
end
