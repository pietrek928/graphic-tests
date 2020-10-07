class rect:
    def dist_point( s, p ):
        p = p-s.p
        v = s.v
        r = v.bvec(
            v.dp( p ),
            v.r90().dp( p ),
            compact=False
        )
        return (abs(r)-s.d).clamp_zero().max()
    def dist_rect( s, r ):
        return s.dist_point( r.p ).min()

class line:
    def dist_point( s, p ):
        p1 = p-s.p.get(1, 0)
        return max( s.v.dp(p1).quad(), p.dist_point(s.p).min() ) # min by default ?

class point( vec ):
    def dist_point( s, p ):
        s, p = s.make_same( s, p )
        return (s-p).dp(r)

