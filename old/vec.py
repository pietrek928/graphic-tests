class vec:
    def __init__( *e, compact=True ):
        s.v = []
        if len(e):
            n = len(e)
            l = []
            for i in e:
                if len(l) < len(i.sl):
                    l = i.sl
            #
    def check_same( s, v ):
        s1 = s.sl
        s2 = v.sl
        if s.sl != v.sl:
            raise ValueError( 'size v1{} != size v2{}'.format(s.sl, v.sl) )
    def make_same( s, v ):
        if s.sl != v.sl:
            ls = len(s.sl)
            lv = len(v.sl)
            if ls > lv:
                if s.sl[1:] == v.sl:
                    return (s, v.milti(s.sl[0]))
                else: raise ValueError( 'can\'t extend size v1{} to size v2{}'.format(s.sl, v.sl) )
            else:
                if v.sl[1:] == s.sl:
                    return (s.milti(v.sl[0]),v)
                else: raise ValueError( 'can\'t extend size v2{} to size v1{}'.format(v.sl, s.sl) )
        else return (s, v)
    def multi( s, n ):
        r = vec()
        for i in s.v:
            r.v.extend( i.multi(n) )
        r.sl = tuple( n, *s.sl )
        return r

