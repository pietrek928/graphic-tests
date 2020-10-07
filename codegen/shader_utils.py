from math import ceil, floor
from typing import Iterable, Optional, Type

from codegen.shader import Mat3, Mat4, Sampler2D, Shader, ShaderInputProperty, ShaderOutputProperty, Tv, Vec, Vec2, \
    Vec3, Vec4, convert_arg, get_context_shader, make_vec4_pos


def _perspective_transform(pt: Vec):
    pos_v = pt[:2]
    z = pt[2]
    return Vec.concat(
        pos_v / z, z, 1.0
    )


def rotate_versors(normal_vecs: Iterable[Vec], mat: Mat3 = None):
    if mat is None:
        mat = Mat3.from_(
            get_context_shader().in_uni('projMatrix', Mat4)
        )

    return tuple(
        (mat * Vec3.from_(n)).vers()
        for n in normal_vecs
    )


def _poly_pow(v, n):
    l = [v]
    for i in range(n - 1):
        l.append(l[int(floor(i / 2))] * l[int(ceil(i / 2))])

    return Vec.concat(*l)


def calc_poly(coeff, v):
    if len(coeff) == 1:
        return coeff[0] * v
    return _poly_pow(v, len(coeff)).dp(coeff)


class VertexShader(Shader):
    def _render_definitions(self):
        yield from self._render_uniforms(fixed_location=False)
        yield from self._render_in(fixed_location=True)
        yield from self._render_out()

    def in_(self, n, t: Type[Tv] = None) -> Tv:
        v = self.find_var(self._input, n, t=t)
        if v is None:
            v = t(n)
            self._input.append(v)
        return v

    def project_point(self, pt: Vec, mat: Optional[Mat4] = None):
        if mat is None:
            mat = self.projMatrix
        pt = mat * make_vec4_pos(pt)
        return _perspective_transform(pt)

    @ShaderOutputProperty(name='gl_Position', type=Vec4, register=False)
    def pos(self, pos_vec):
        return make_vec4_pos(pos_vec)

    @ShaderInputProperty
    def vertCoord(self):
        return self.in_('vertCoord', Vec3)

    @ShaderInputProperty
    def projMatrix(self):
        return self.in_uni('projMatrix', Mat4)

    @ShaderInputProperty
    def vertNorm(self):
        return self.in_('vertNorm', Vec3)

    @ShaderInputProperty
    def vertColl(self):
        return self.in_('vertColl', Vec3)

    @ShaderOutputProperty(type=Vec3)
    def fragNorm(self, value: Vec3):
        return value

    @ShaderOutputProperty(type=Vec3)
    def fragColl(self, value: Vec3):
        return value

    @ShaderInputProperty
    def vertTexCoord(self):
        return self.in_('vertTexCoord', Vec2)

    @ShaderOutputProperty(type=Vec2)
    def fragTexCoord(self, value: Vec2):
        return value


class FragmentShader(Shader):
    def _render_definitions(self):
        yield from self._render_uniforms(fixed_location=True)
        yield from self._render_in(fixed_location=False)
        yield from self._render_out()

    def in_(self, n, t: Type[Tv] = None) -> Tv:
        v = self.find_var(self._input, n, t=t)
        if v is not None:
            return v
        raise ValueError(f'Requested undefined fragment input `{n}`')

    @ShaderInputProperty
    def fragNorm(self):
        return self.in_('fragNorm', Vec3)

    @ShaderInputProperty
    def fragColl(self):
        return self.in_('fragColl', Vec3)

    @ShaderInputProperty
    def fragTexCoord(self):
        return self.in_('fragTexCoord', Vec2)

    @ShaderOutputProperty(type=Vec4)
    def color(self, value: Vec):
        return Vec4.from_(value)

    @ShaderInputProperty
    def colorTexture(self):
        return self.in_uni('colorTexture', Sampler2D)


# TODO: textured light, animated ?
class LightSource:
    def __init__(self, n, has_source=False, params=()):
        self._n = n
        self._params = dict(params)
        self._has_source = has_source

    def _get_param(self, param_name):
        if param_name in self._params:
            return convert_arg(self._params[param_name])
        return get_context_shader().in_uni(f'{self._n}_{param_name}', Vec3)

    def _intensity_from_light_vector(self, n, light_versor):
        proj_len = light_versor.dp(n)  # assume hiding cull face is on
        reflect_intensity = (
            # z is reversed - we add -
            # -(b - (a-b)) = a - 2b
                light_versor[2] - n[2] * (proj_len * 2.)
        )
        return proj_len.max(0.) + calc_poly(
            self._get_param('reflect_poly'), reflect_intensity.max(0.)
        )

    def calc_color_for_pixel(self, n, pixel_pos=None):
        if self._has_source:
            if pixel_pos is not None:
                raise ValueError('Sourced light needs pixel position in calculation')
            light_pos = self._get_param('pos')
            light_vec = light_pos - pixel_pos  # get_context_shader().get_pixel_coord()
            light_dir = light_vec.vers()
            light_strength = (
                    self._intensity_from_light_vector(n, light_dir) / light_vec.dist().max(.5)
            )
        else:
            light_strength = self._intensity_from_light_vector(n, self._get_param('light_vec'))
        return self.color * light_strength

    @property
    def color(self):
        return self._get_param('color')


test_light = LightSource(
    'TestLight',
    params=dict(
        color=(0., 1., 0.),
        reflect_poly=(.2, .5, .1),
        light_vec=(1., 1., 1.)
    )
)


class SimpleVertexShader(VertexShader):
    def _gen_code(self):
        self.pos = self.project_point(
            make_vec4_pos(
                self.vertCoord
            )
        )
        self.fragNorm, self.fragColl = rotate_versors((
            self.vertColl,
            self.vertNorm,
        ))

        self.fragTexCoord = self.vertTexCoord


class SimpleFragmentShader(FragmentShader):
    def _gen_code(self):
        cv = self.fragColl
        test_col = test_light.calc_color_for_pixel(self.fragNorm)
        self.color = test_col * self.colorTexture.sample(self.fragTexCoord)


def test():
    vert_shader = SimpleVertexShader()
    vert_content = vert_shader.gen()
    with open('a.vert', 'w') as f:
        f.write(vert_content)

    frag_shader = SimpleFragmentShader(
        inputs=vert_shader.output_params, uniform_inputs=vert_shader.uniform_params
    )
    frag_content = frag_shader.gen()
    with open('a.frag', 'w') as f:
        f.write(frag_content)


test()
