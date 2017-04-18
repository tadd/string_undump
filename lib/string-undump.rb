class String
  def undump_badly
    hex = /[0-9a-fA-F]/
    esctable = {
      '\n' => "\n",
      '\r' => "\r",
      '\t' => "\t",
      '\f' => "\f",
      '\v' => "\v",
      '\b' => "\b",
      '\a' => "\a",
      '\e' => "\e",
    }
    e = self.encoding
    s = if self[0] == '"' && self[-1] == '"'
        self[1..-2]
      else
        self.dup
      end
    s.gsub!(/\\\#\$/, '#$')
    s.gsub!(/\\\#@/, '#@')
    s.gsub!(/\\\#{/, '#{')
    s.gsub!(/\\"/, '"')
    s.gsub!(/\\\\/, '\\')
    s.gsub!(/\\[nrtfvbae]/) {|m| esctable[m]}
    s.gsub!(/\\u#{hex}{4}/) {|m| m[2..-1].hex.chr(e)}
    s.gsub!(/\\[ux]{#{hex}+}/) {|m| m[3..-1].hex.chr(e)}
    s.gsub!(/(?:\\x#{hex}{2})+/) {|m|
      m.gsub(/\\x/, '').scan(/../).map(&:hex).pack("C*").force_encoding(e)
    }
    s
  end

  alias_method(:undump, :undump_badly) unless self.new.respond_to?(:undump)
end
