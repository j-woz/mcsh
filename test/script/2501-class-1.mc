
# TEST:SKIP

class C {
  static s
  = s 90
  function static {
    = s 100
  }
  public x
  = x 42    # public
  = y 43    # private
  function new { t } {
    = y $t
  }
  function f { v } {
    print $v
    = x $v
  }
}

= x [ new $C ]
$x f 47
$x f 0

class D extends $C {
  function g { } {
    print g
  }
  function f { t } {
    super f $t
  }
}

# Local Variables:
# mode: sh
# End:
