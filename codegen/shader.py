from _contextvars import ContextVar
from collections import defaultdict
from inspect import getfullargspec
from typing import Dict, Iterable, List, Optional, Tuple, Type, TypeVar

_shader_context: ContextVar['Shader'] = ContextVar('gen_context', default=None)

Tv = TypeVar('Tv')


class Var:
    typename = None

    def __init__(self, expr=None):
        if expr is None:
            expr = get_context_shader().gen_var_name(prefix=self.typename[0])
        elif not isinstance(expr, str):
            expr = self.parse_expr(expr)
        self._expr = expr

    def __str__(self):
        if isinstance(self._expr, str):
            return self._expr
        return self.format_const(self._expr)

    @classmethod
    def parse_expr(cls, expr):
        return expr

    @property
    def type(self):
        return self.__class__

    @property
    def name(self):
        return self._expr

    @property
    def isconst(self):
        return not isinstance(self._expr, str)

    def istype(self, *t):
        return isinstance(self, t)

    def _call_op(self, op_n, v, rt=None, is_func=False):
        v = convert_arg(v)
        if rt is None:
            if not v.istype(self.type):
                raise ValueError(f'Cannot do {op_n} with {self.typename} and {v.typename}')
            rt = self.type
        if is_func:
            return get_context_shader().add_function_call(rt, op_n, self, v)
        else:
            return get_context_shader().add_operator_call(rt, op_n, self, v)

    def __add__(self, v):
        return self._call_op('+', v)

    def __sub__(self, v):
        return self._call_op('-', v)

    def __mul__(self, v):
        return self._call_op('*', v)

    def __truediv__(self, v):
        return self._call_op('/', v)

    def max(self, v):
        return self._call_op('max', v, is_func=True)

    def min(self, v):
        return self._call_op('min', v, is_func=True)

    @classmethod
    def format_const(cls, v):
        return str(v)


class Float(Var):
    typename = 'float'
    len = 1

    @classmethod
    def parse_expr(cls, expr):
        return float(expr)

    def inv(self):
        return convert_arg(1.) / self

    def invsqrt(self):
        return get_context_shader().add_function_call(self.type, 'inversesqrt', self)

    def iszero(self):
        return self._expr == 0.

    def isone(self):
        return self._expr == 1.

    def isminusone(self):
        return self._expr == -1.

    @classmethod
    def noise(cls: Type[Tv], v) -> Tv:
        return get_context_shader().add_function_call(cls, f'noise1', v)


class Half(Var):
    typename = 'half'


def _slice_range(s: slice, array_len) -> range:
    step = s.step
    if not step:
        step = 1
    start = s.start
    if start is None:
        start = 0 if step > 0 else array_len - 1
    stop = s.stop
    if stop is None:
        stop = array_len if step > 0 else -1

    return range(start, stop, step)


class Vec(Var):
    typename = 'vec'
    len = 0
    ELEM_NAMES = 'xyzw'

    @classmethod
    def parse_expr(cls, expr):
        expr = convert_args(expr)
        assert len(expr) == cls.len
        return expr

    @property
    def isaddsub(self):
        return self.isconst and all(
            v.iszero or v.isone or v.isminusone
            for v in self._expr
        )

    @classmethod
    def from_(cls, v):
        if v.type.len == cls.len:
            return v
        elif v.type.len < cls.len:
            zeros = [0.] * (cls.len - v.type.len)
            return cls.concat(v, *zeros)
        else:
            return v[:cls.len]

    @classmethod
    def noise(cls: Type[Tv], v) -> Tv:
        return get_context_shader().add_function_call(cls, f'noise{cls.len}', v)

    def __len__(self):
        return self.len

    def item(self, n) -> Var:
        if n < 0:
            n += self.len
        if not 0 <= n < self.len:
            raise IndexError(f'Invalid index {n} for {self.typename}')
        if self.isconst:
            return convert_arg(self._expr[n])
        return Float(f'{self}.{self.ELEM_NAMES[n]}')

    def __getitem__(self, pos):
        if isinstance(pos, int):
            return self.item(pos)
        elif isinstance(pos, slice):
            pos_range = _slice_range(pos, self.len)
            if pos_range.start == 0 and pos_range.step == 1:
                return VEC_BY_LEN[pos_range.stop](
                    f'{self}.{self.ELEM_NAMES[:pos_range.stop]}'
                )
            return self.concat(
                *map(self.item, pos_range)
            )
        elif isinstance(pos, tuple):
            return self.concat(*(
                self.item(v) for v in pos
            ))

    @staticmethod
    def concat(*args):
        args = convert_args(args)
        if len(args) == 1:
            return args[0]

        out_len = sum(map(lambda v: v.type.len, args))
        out_type = VEC_BY_LEN.get(out_len)
        if out_type is None:
            raise ValueError(f'Invalid vector length {out_len}')
        return get_context_shader().add_function_call(out_type, out_type.typename, *args)

    @classmethod
    def format_const(cls, v):
        vv = ', '.join(map(str, v))
        return f'{cls.typename}({vv})'

    def dp(self, v):
        assert v.istype(Vec)
        w = self
        if w.type.len < v.type.len:
            v = v[:w.type.len]
        elif w.type.len > v.type.len:
            w = w[:v.type.len]
        if not v.istype(self.type):
            raise ValueError(f'Cannot do `dp` with {self.typename} and {v.typename}')

        # if self.isaddsub or v.isaddsub:
        #

        return get_context_shader().add_function_call(Float, 'dot', w, v)

    def __mul__(self, v):
        v = convert_arg(v)
        rt = None
        if v.istype(Float):
            rt = self.type
        return self._call_op('*', v, rt=rt)

    def __truediv__(self, v):
        v = convert_arg(v)
        rt = None
        if v.istype(Float):
            rt = self.type
        return self._call_op('/', v, rt=rt)

    def qdist(self):
        return self.dp(self)

    def dist(self):
        return get_context_shader().add_function_call(self.type, 'length', self)

    def vers(self):
        return get_context_shader().add_function_call(self.type, 'normalize', self)


class Vec2(Vec):
    typename = 'vec2'
    len = 2


class Vec3(Vec):
    typename = 'vec3'
    len = 3


class Vec4(Vec):
    typename = 'vec4'
    len = 4


class Mat(Var):
    typename = 'mat'
    s = 0

    @classmethod
    def from_(cls, *a):
        if not a:
            a = (1.,)
        a = convert_args(a)
        if len(a) == 1:
            assert a[0].istype(Float, Mat)
            return get_context_shader().add_function_call(cls, cls.typename, a[0])

        raise ValueError(f'Matrix creation from {a} unsupported')

    def item(self, n):
        if n < 0:
            n += self.s
        if not 0 <= n < self.s:
            raise IndexError(f'Invalid index {n} for {self.typename}')
        rt = VEC_BY_LEN[self.s]
        return rt(n=f'{self}[{n}]')

    def __mul__(self, v):
        if v.istype(Mat):
            return self._call_op('*', v)
        elif v.istype(Vec):
            assert self.type.s == v.type.len
            return self._call_op('*', v, rt=v.type)
        else:
            raise ValueError(f'Unsupported multiplication of {self} and {v}')


class Mat2(Mat):
    typename = 'mat2'
    s = 2


class Mat3(Mat):
    typename = 'mat3'
    s = 3


class Mat4(Mat):
    typename = 'mat4'
    s = 4


VEC_BY_LEN = {
    1: Float,
    2: Vec2,
    3: Vec3,
    4: Vec4
}


class Sampler2D(Var):
    typename = 'sampler2D'

    # TODO: type opts ?
    def __init__(self, expr: str, components=3):
        super().__init__(expr)
        self._rt = VEC_BY_LEN.get(components)
        if self._rt is None:
            raise ValueError(f'Wrong number of components {components}')

    def sample(self, coords: Vec2):
        return self._rt.from_(
            get_context_shader().add_function_call(
                Vec4, 'texture', self, coords
            )
        )


def get_context_shader() -> 'Shader':
    shader = _shader_context.get(None)
    assert shader is not None, 'No shader in current context'
    return shader


def make_vec4_pos(v: Vec):
    if isinstance(v, Vec4):
        return v
    elif isinstance(v, Vec3):
        return Vec4.concat(v, 1.)
    elif isinstance(v, Vec2):
        return Vec4.concat(v, 0., 1.)
    elif isinstance(v, Float):
        return Vec4.concat(v, 0., 0., 1.)
    else:
        raise ValueError(f'Conversion unsupported for type {v.typename}')


def to_const(v):
    if isinstance(v, float):
        return Float(v)
    elif isinstance(v, (tuple, list)):
        if len(v) == 1:
            return Float(v[0])
        elif len(v) in VEC_BY_LEN:
            return VEC_BY_LEN[len(v)](v)

    raise ValueError(f'Unsupported conversion to const of {v}')


def convert_arg(a) -> Var:
    return a if isinstance(a, Var) else to_const(a)


def convert_args(arg_list) -> Tuple[Var, ...]:
    return tuple(
        map(convert_arg, arg_list)
    )


class ShaderInputProperty:
    _name = None
    _args = ()

    def __init__(self, *args, **kwargs):
        if len(args) == 1 and not kwargs and callable(args[0]):
            self._set_func(args[0])
        else:
            self._setup(*args, **kwargs)

    def __call__(self, func):
        self._set_func(func)
        return self

    def _set_func(self, func):
        self._func = func
        if not self._name:
            self._name = func.__name__
        argspec = getfullargspec(func)
        func_args = tuple(argspec.args)
        assert func_args[0] == 'self'
        self._args = func_args[1:]

    def _setup(self, name=None, func=None):
        if func is not None:
            self._name = name
            self._set_func(func)

    def __get__(self, shader: 'Shader', objtype=None):

        if shader is None:
            return self

        try:
            return shader.computed_props[self._name]
        except KeyError:
            get_func_kwargs = {
                n: getattr(shader, n)
                for n in self._args
            }
            value = self._func(shader, **get_func_kwargs)
            shader.computed_props[self._name] = value
            return value

    def __set__(self, shader: 'Shader', value):
        raise ValueError(f'Cannot set shader input property `{self._name}` for `{shader.__class__.__name__}`')


class ShaderOutputProperty:
    _name = None
    _type = None
    _func = None
    _register = True

    def __init__(self, *args, **kwargs):
        if len(args) == 1 and not kwargs and callable(args[0]):
            self._set_func(args[0])
        else:
            self._setup(*args, **kwargs)

    def __call__(self, func):
        self._set_func(func)
        return self

    def _set_func(self, func):
        self._func = func
        if not self._name:
            self._name = func.__name__
        argspec = getfullargspec(func)
        func_args = tuple(argspec.args)
        assert func_args[0] == 'self'

    def _setup(self, name=None, func=None, type: Optional[Type[Var]] = None, register=True):
        self._name = name
        if func is not None:
            self._set_func(func)
        self._type = type
        self._register = register

    def __get__(self, shader: 'Shader', objtype=None):
        raise ValueError(f'Cannot get shader output property `{self._name}` from `{shader.__class__.__name__}`')

    def __set__(self, shader: 'Shader', value):

        if shader is None:
            raise ValueError(f'Setting property {self._name} on class is not allowed')

        value = convert_arg(value)

        if self._func is not None:
            value = self._func(shader, value)

        if self._type is not None:
            if not value.istype(self._type):
                raise ValueError(f'Output property {self._name} should have type `{self._type}`, but got {value.type}')

        shader.out_(self._name, value, register=self._register)


class Shader:
    def __init__(self, inputs: Iterable[Var] = (), uniform_inputs: Iterable[Var] = (), version=330):
        self._version = version
        self._input: List[Var] = list(inputs)
        self._uniform_input: List[Var] = list(uniform_inputs)
        self._output: List[Var] = []
        self._code_lines = []
        self._name_counter = defaultdict(int)
        self.computed_props: Dict[str, Var] = {}

    def supports_version(self, v: int):
        return v <= self._version

    @property
    def uniform_params(self) -> Tuple[Var]:
        return tuple(self._uniform_input)

    @property
    def output_params(self) -> Tuple[Var]:
        return tuple(self._output)

    @staticmethod
    def find_var(vars: Iterable[Var], n, t=None) -> Optional[Var]:
        for var in vars:
            if var.name == n:
                if t is not None:
                    if not var.istype(t):
                        raise ValueError(f'Input variable `{n}` with type {t} requested, but has type {var.type}')
                return var

    def in_(self, n, t: Type[Tv] = None) -> Tv:
        raise NotImplementedError(f'Input unimplemented for shader {self.__class__}')

    def in_uni(self, n: str, t: Type[Var] = Float):
        v = self.find_var(self._uniform_input, n, t=t)
        if v is None:
            v = t(n)
            self._uniform_input.append(v)
        return v

    def out_(self, n: str, v: Var, register=True):
        if self.find_var(self._output, n) is not None:
            raise ValueError(f'Output var `{n}` already set')
        self._code_lines.append(f'{n} = {v};')
        if register:
            self._output.append(
                v.type(n)
            )

    def gen_var_name(self, prefix=None):
        prefix = prefix or 'v'
        n = f'{prefix}{self._name_counter[prefix]}'
        self._name_counter[prefix] += 1
        return n

    def add_function_call(self, rt: Type[Var], func_name, *func_args):
        rv = rt()
        call_args = ', '.join(map(str, func_args))
        self._code_lines.append(
            f'{rv.typename} {rv} = {func_name}({call_args});'
        )
        return rv

    def add_operator_call(self, rt: Type[Var], op_name, op_arg1, op_arg2):
        rv = rt()
        self._code_lines.append(
            f'{rv.typename} {rv} = {op_arg1} {op_name} {op_arg2};'
        )
        return rv

    def _gen_code(self):
        pass

    def _render_uniforms(self, fixed_location=True):
        for i, uni_v in enumerate(self._uniform_input):
            if fixed_location and self.supports_version(430):
                yield f'layout(location = {i}) uniform {uni_v.typename} {uni_v};'
            else:
                yield f'uniform {uni_v.typename} {uni_v};'

    def _render_in(self, fixed_location=True):
        for i, in_v in enumerate(self._input):
            if fixed_location and self.supports_version(430):
                yield f'layout(location = {i}) in {in_v.typename} {in_v};'
            else:
                yield f'in {in_v.typename} {in_v};'

    def _render_out(self):
        for out_v in self._output:
            yield f'out {out_v.typename} {out_v};'

    def _render_definitions(self):
        return ()

    def _render_content(self):
        self._gen_code()
        yield f'#version {self._version}'
        yield from self._render_definitions()
        yield 'void main() {'
        yield from self._code_lines
        yield '}'

    def gen(self):
        old_val = _shader_context.set(self)
        try:
            return '\n'.join(self._render_content())
        finally:
            _shader_context.reset(old_val)
