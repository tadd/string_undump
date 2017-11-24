require 'test-unit'

require_relative '../lib/string_undump'

class TestUndump < Test::Unit::TestCase
  def test_undump_badly
    assert_equal('foo', '"foo"'.undump_badly)
    assert_equal('foo#$bar#@baz#{quxx}', 'foo\#$bar\#@baz\#{quxx}'.undump_badly)
    assert_equal('\\', '\\\\'.undump_badly)
    assert_equal(%(\\"), '\\\\"'.undump_badly)
    assert_equal("\n", '\n'.undump_badly)
    assert_equal(%(\\"\n), '\\\\\\"\n'.undump_badly)
    assert_equal('すごーい', '\u3059\u3054\u30FC\u3044'.undump_badly)
    assert_equal('たのしー', '\xE3\x81\x9F\xE3\x81\xAE\xE3\x81\x97\xE3\x83\xBC'.undump_badly)
    assert_equal('🐾', '\u{1F43E}'.undump_badly)
    assert_equal(%(すごーい\\🐾たのしー\n\#{foo}),
                 ('"\u3059\u3054\u30FC\u3044\\\\\u{1F43E}' +
                  '\xE3\x81\x9F\xE3\x81\xAE\xE3\x81\x97\xE3\x83\xBC\\n\#{foo}"').undump_badly)
  end

  def test_undump_roughly
    assert_equal('foo', '"foo"'.undump_roughly)
    assert_equal('foo#$bar#@baz#{quxx}', 'foo\#$bar\#@baz\#{quxx}'.undump_roughly)
    assert_equal('\\', '\\\\'.undump_roughly)
    assert_equal(%(\\"), '\\\\"'.undump_roughly)
    assert_equal("\n", '\n'.undump_roughly)
    assert_equal(%(\\"\n), '\\\\\\"\n'.undump_roughly)
    assert_equal('すごーい', '\u3059\u3054\u30FC\u3044'.undump_roughly)
    assert_equal('\\a\\', '\\\\a\\\\'.undump_roughly)
    assert_equal("\nn", '\nn'.undump_roughly)
    assert_equal("\u30593059", '\u30593059'.undump_roughly)
    assert_equal('たのしー', '\xE3\x81\x9F\xE3\x81\xAE\xE3\x81\x97\xE3\x83\xBC'.undump_roughly)
    assert_equal('🐾', '\u{1F43E}'.undump_roughly)
    assert_equal(%(すごーい\\🐾たのしー\n\#{foo}),
                 ('"\u3059\u3054\u30FC\u3044\\\\\u{1F43E}' +
                  '\xE3\x81\x9F\xE3\x81\xAE\xE3\x81\x97\xE3\x83\xBC\\n\#{foo}"').undump_roughly)
  end

  def test_undump_roughly_error
    assert_raise(ArgumentError.new('invalid Unicode escape')) {'\u'.undump_roughly}
    assert_raise(ArgumentError.new('unterminated Unicode escape')) {'\u{'.undump_roughly}
    assert_raise(ArgumentError.new('unterminated Unicode escape')) {'\u{3042'.undump_roughly}
    assert_raise(ArgumentError.new('invalid hex escape')) {'\x'.undump_roughly}
    assert_equal('#', '\#'.undump_roughly)
    assert_raise(ArgumentError.new('invalid escape')) {'\\'.undump_roughly}
  end

  def test_undump_taintedness
    assert_true(''.taint.undump.tainted?)
  end

  def test_undump
    assert_includes(String.instance_methods, :undump)
  end
end
