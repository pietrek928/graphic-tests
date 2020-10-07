#!/usr/bin/python

import sys
from lxml import etree

class pt:
    def __init__( s, _x, _y, _r=0 ): #TODO: layer, net
        s.x = float(_x)
        s.y = float(_y)
        s.r=_r
    def quad_dist( s, p2 ):
        a = s.x-p2.x;
        b = s.y-p2.y;
        return a*a+b*b;
    def m( s ):
        return pt( -s.x, s.y, s.r )

class pt_set:
    def __init__( s ):
        s.h_tab = {}
        s.pt_tab = []

    def add_pt( s, x, y, r ):
        hh = str(x)+'#'+str(y)
        p = s.h_tab.find(hh)
        r = float(r)
        if p==None:
            new_p = pt( float(x), float(y) )
            new_p.n = len( s.pt_tab )
            new_p.r = float( r )
            s.h_tab[hh] = new_p
            s.pt_tab.append( new_p )
            p = new_p
        else:
            if p.r<r: p.r=r
            p.hh = hh
        return p.n

class fp:
    def __init__( s, _name ):
        s.name = _name
        s.coord = pt_set()
        s.pads = []
    def add_conn( s, n, c ):
        pads.append( n )
    def add_point( s, x, y, r, layer='None' ):
        return s.p.add_pt( x, y, r, layer )
    def add_track( s, layer, p1, p2, w ):
        o = brd_obj();
        o.t = 'track'
        o.l = int(layer)
        o.p1 = p1
        o.p2 = p2
        o.w = float(w)
        obj_tab.append(o)
    def add_circ( s, layer, p1, r ):
        o = brd_obj();
        o.t = 'circ'
        o.l = int(layer)
        o.p1 = p1
        o.r = float(r)
    def add_rect( s, layer, p1, dx, dy, rot ):
        o = brd_obj();
        o.t = 'rect'
        o.dx = float(dx)
        o.dy = float(dy)
        o.rot = float(rot)
    def insert( s, os, on, m ):
        n = os.new_fp( s.name, on )
        for p in s.coord.pt_tab:
            os.fp_add_pt( p )
        for ( ln , l ) in p.L:
            if m: ln = layer_rev[ ln ]
            for p in l.pt_tab:
                os.fp_add_circ( ln, p.p1, p.r )
        for o in s.obj_tab:
            l = o.l
            if m:
                l = layer_rev[ l ]
            if o.t=='track':
                os.fp_add_track( l, o.p1, o.p2, o.w )
            elif o.t=='circ':
                os.fp_add_track( l, o.p1, o.r )
            elif o.t=='rect':
                os.fp_add_track( l, o.p1, o.dx, o.dy, o.rot )

def board_desr( obj_set ):
    wsp_tab = []
#    def add_mv_point():
#        /
#    def add_st_point():
#        p =
#
#    def add_track():
#        /
#    def add_via():
#        /
#    def add_hole():
#        /
    def fp_place( s, nfp, coord ):
        os.add_fp_coord( nfp, coord.x, coord.y, coord.rot )
# TODO: insert pads

G__top_num = 1;
G__bot_num = 16;

IN__fname = sys.argv[1]

tree = etree.parse( IN__fname, etree.XMLParser(ns_clean=True) )
r = tree.getroot()

board_descr = r.find( 'drawing' ).find( 'board' )

l = board_descr.find( 'libraries' )
for ii in l:
    __n = ii.get('name')+'#'
    for jj in ii:
        for p in jj:
            nn = __n+str( p.get( 'name' ))
            fp = fp_list[nn] = obj_set( nn );
            for e in p:
                if e.tag == 'wire':
                    npt1 = fp.add_point( e.get('x1'), e.get('y1') )
                    npt2 = fp.add_point( e.get('x2'), e.get('y2') )
                    fp.add_track( e.get('layer'), npt1, npt2, e.get('width') )
                elif e.tag == 'pad':
                    npt = fp.add_point( e.get('x'), e.get('y') )
                    fp.add_conn( G__top_num, e.get('name'), npt )
                    fp.add_conn( G__bot_num, e.get('name'), npt )
                    fp.add_circ( G__top_num, e.get('drill') )
                    fp.add_circ( G__bot_num, e.get('drill') )
                elif e.tag == 'smd':
                    npt = fp.add_point( e.get('x'), e.get('y') )
                    fp.add_conn( fp.get('layer'), e.get('name'), npt )
                    fp.add_rect( npt, e.get('dx'), e.get('dy'), e.get('rot') )
                elif e.tag == 'circle':
                    fp.add_circ( G__top_num, e.get('radius') )
                elif e.tag == 'rectangle':
                    fp.add_rect( npt, e.get('dx'), e.get('dy'), e.get('rot') )
                else:
                    print( 'Warning: unknown tag '+e.tag )

print( "" )

s = board_descr.find( 'signals' )
for ss in s:
    __n = ss.get( 'name' )+'#'
    for e in ss:
        if e.tag == 'wire':
            npt1 = G__objects.add_point_mv( e.get('layer'), e.get('x1'), e.get('y1') )
            npt2 = G__objects.add_point_mv( e.get('layer'), e.get('x2'), e.get('y2') )
            G__objects.add_track( e.get('layer'), npt1, npt2, e.get('width') )
        elif: e.tag == 'via':
            nmv = G__objects.add_mv( e.get('x'), e.get('y') )
            [ l1, l2 ] = e.get('extent').split('-')
            npt1 = G__objects.add_point_mv( l1, e.get('x'), e.get('y') )
            npt2 = G__objects.add_point_mv( l2, e.get('x'), e.get('y') )
        elif: e.tag == 'contactref':
        else:
            print( 'Warning: unknown tag '+e.tag )

e = board_descr.find( 'elements' )
for ee in e:
    nn = ee.get('library')+'##'+ee.get('package')
    G__objects.add_fp( fp_list[nn], ee.get('x'), ee.get('y'), ee.get('rot') )


