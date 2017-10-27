# encoding: euc-jp
require 'test-unit'

require_relative '../lib/string_undump'

class TestUndumpEucJp < Test::Unit::TestCase
  def test_undump_badly
    assert_equal('すごーい', '\xA4\xB9\xA4\xB4\xA1\xBC\xA4\xA4'.undump_badly)
  end
end
