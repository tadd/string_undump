# encoding: euc-jp
require 'test-unit'

require_relative '../lib/string_undump'

class TestUndumpEucJp < Test::Unit::TestCase
  def test_undump_badly
    assert_equal('すごーい', '\xA4\xB9\xA4\xB4\xA1\xBC\xA4\xA4'.undump_badly)
    assert_equal('たのしー', '\x{A4BF}\x{A4CE}\x{A4B7}\x{A1BC}'.undump_badly)
  end
end
