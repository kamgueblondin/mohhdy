#!/usr/bin/env python3
"""Lot 93 model of the CSS math functions the reader now simplifies:
sin(), cos(), tan(), asin(), acos(), atan(), atan2(), pow(), sqrt(),
hypot(), log(), exp() and the constants e, pi, infinity, -infinity, nan.

This file does not share code with the C under test
(userspace/wf_cssprop.c). Values come in and go out as integers in
1/10000 of the canonical unit (a number, deg for angles, px, %, s), the
way scripts/lot92_mathgen.py keeps them. The exact result is computed with
Python's decimal module at 60 digits (pi from Machin's formula, sin and
cos from their Taylor series after an exact reduction, atan with two
argument halvings, Decimal.exp, Decimal.ln and Decimal.sqrt), or exactly
with integers and fractions (sqrt, hypot, pow with an integer exponent),
then rounded half away from zero to 1/10000. A value past 100000 units is
not known (None).

The C side works in 62-bit fixed point for the transcendental functions,
so its result before rounding is within about 1e-12 (relative) of the
exact one. A result whose exact value lies within MARGIN of a
rounding boundary (x.xxxx5) may round either way there (round_units
raises Ambiguous); the generators do
not use such cases and count them. Exact ties that a pow() with an
integer exponent gives (pow(0.5, 5) = 0.03125 -> 0.0313) are exact on both
sides and are kept.

Rules (CSS Values 4, restricted to the reader's 1/10000 values):
* constants: e = 2.7183, pi = 3.1416 (each value is kept in 1/10000, so
  calc(pi * 1000) is 3141.6); infinity, -infinity and nan are numbers that
  are never known (the text is kept).
* sin, cos, tan take a number (radians, known when |x| <= 100) or an angle
  (deg, any size; reduced exactly), and give a number; tan at an odd
  multiple of 90deg is infinite (not known).
* asin, acos take a number in [-1, 1] (else NaN, not known), atan any
  number; they give an angle in deg (asin, atan in [-90, 90], acos in
  [0, 180]). atan2(A, B) takes two values of one type and gives the angle
  of the point (B, A): atan2(0, 0) = 0, atan2(0, -1) = 180deg.
* pow(A, B), sqrt(A), log(A), log(A, B), exp(A) take numbers and give a
  number; hypot(A, ...) takes values of one type and gives that type.
  pow: B = 0 gives 1; A = 0 gives 0 for B > 0 and infinity for B < 0;
  A < 0 with a B that is not an integer is NaN. sqrt of a negative, log of
  zero or a negative, a base of zero, a negative or one are not known.
"""
from decimal import Decimal as D, getcontext, ROUND_FLOOR
from fractions import Fraction as F
import math

getcontext().prec = 60
S = 10000
LIM = 10 ** 9
MARGIN = D("1e-11")   # relative to max(1, |value|), in the value's unit


def _atan_inv(k):
    x = D(k)
    x2 = x * x
    t = 1 / x
    s = D(0)
    n = 1
    sign = 1
    while t / n > D(10) ** -70:
        s += sign * t / n
        t /= x2
        n += 2
        sign = -sign
    return s


PI = 16 * _atan_inv(5) - 4 * _atan_inv(239)
E = D(1).exp()
CONST = {"e": 27183, "pi": 31416, "infinity": None, "-infinity": None, "nan": None}


class Ambiguous(Exception):
    """the exact value is too close to a rounding boundary"""


def dec(fr):
    return D(fr.numerator) / D(fr.denominator)


def round_units(y):
    """y (Decimal, the value) -> integer in 1/10000, half away from zero;
    raises Ambiguous near a boundary; None past the limit"""
    a = abs(y) * S
    fl = a.to_integral_value(rounding=ROUND_FLOOR)
    frac = a - fl
    if abs(frac - D("0.5")) < MARGIN * S * max(D(1), abs(y)):
        raise Ambiguous()
    r = int(fl) + (1 if frac >= D("0.5") else 0)
    if y < 0:
        r = -r
    return r if -LIM <= r <= LIM else None


def _sin_series(x):
    s = D(0)
    t = x
    n = 1
    while abs(t) > D(10) ** -65:
        s += t
        t = -t * x * x / ((n + 1) * (n + 2))
        n += 2
    return s


def _cos_direct(x):
    s = D(0)
    t = D(1)
    n = 0
    while abs(t) > D(10) ** -65:
        s += t
        t = -t * x * x / ((n + 1) * (n + 2))
        n += 2
    return s


def _reduce(x):
    """x (Decimal radians) into [-pi, pi]"""
    k = (x / (2 * PI)).to_integral_value()
    return x - k * 2 * PI


def _sin(x):
    return _sin_series(_reduce(x))


def _cos(x):
    return _cos_direct(_reduce(x))


def _atan(x):
    if x < 0:
        return -_atan(-x)
    if x > 1:
        return PI / 2 - _atan(1 / x)
    # two halvings: atan x = 2 atan(x / (1 + sqrt(1 + x^2)))
    for _ in range(2):
        x = x / (1 + (1 + x * x).sqrt())
    s = D(0)
    t = x
    n = 1
    sign = 1
    while abs(t) / n > D(10) ** -65:
        s += sign * t / n
        t *= x * x
        n += 2
        sign = -sign
    return 4 * s


def _atan2(y, x):
    """y, x Decimal; result in radians"""
    if x == 0 and y == 0:
        return D(0)
    if x == 0:
        return PI / 2 if y > 0 else -PI / 2
    a = _atan(y / x)
    if x > 0:
        return a
    return a + PI if y >= 0 else a - PI


def rad_to_deg(r):
    return r * 180 / PI


def trig(name, ang, v):
    """sin, cos or tan of v (1/10000 deg when ang, else 1/10000 rad)"""
    if v is None:
        return None
    if ang:
        t = v % 3600000                        # exact, [0, 360deg)
        if name == "tan" and t % 900000 == 0 and (t // 900000) % 2 == 1:
            return None                        # a pole
        if t % 900000 == 0:
            q = t // 900000                    # exact values on the axes
            val = {"sin": [0, 1, 0, -1], "cos": [1, 0, -1, 0], "tan": [0, None, 0, None]}[name][q]
            return val * S
        x = D(t) / S * PI / 180
    else:
        if abs(v) > 1000000:
            return None
        x = D(v) / S
    s, c = _sin(x), _cos(x)
    if name == "sin":
        return round_units(s)
    if name == "cos":
        return round_units(c)
    return round_units(s / c)


def arc(name, v):
    """asin, acos or atan of v (1/10000) in 1/10000 deg"""
    if v is None:
        return None
    x = D(v) / S
    if name == "atan":
        return round_units(rad_to_deg(_atan(x)))
    if abs(v) > S:
        return None
    c = (1 - x * x).sqrt()
    if name == "asin":
        return round_units(rad_to_deg(_atan2(x, c)))
    return round_units(rad_to_deg(_atan2(c, x)))


def atan2(a, b):
    if a is None or b is None:
        return None
    return round_units(rad_to_deg(_atan2(D(a), D(b))))


def isqrt_round(n):
    """sqrt(n) rounded half away from zero, n an integer >= 0"""
    s = math.isqrt(n)
    return s + 1 if n - s * s > s else s


def sqrt(v):
    if v is None or v < 0:
        return None
    r = isqrt_round(v * S)
    return r if r <= LIM else None


def hypot(vals):
    if any(v is None for v in vals):
        return None
    r = isqrt_round(sum(v * v for v in vals))
    return r if r <= LIM else None


def pow_(a, b):
    if a is None or b is None:
        return None
    if b % S == 0:
        n = b // S
        if n == 0:
            return S
        if a == 0:
            return 0 if n > 0 else None
        # far past the limit or far below 1/20000: no need for the exact power
        mag = n * (D(abs(a)) / S).log10()
        if mag > 6:
            return None
        if mag < -6:
            return 0
        r = round_frac(F(a, S) ** n * S)
        return r if -LIM <= r <= LIM else None
    if a < 0:
        return None
    if a == 0:
        return 0 if b > 0 else None
    t = D(b) / S * (D(a) / S).ln()
    if t > 12:
        return None
    return round_units(t.exp())


def round_frac(x):
    """a Fraction rounded half away from zero"""
    if x >= 0:
        return int(math.floor(x + F(1, 2)))
    return -int(math.floor(-x + F(1, 2)))


def log(a, b=None, hasb=False):
    if a is None or (hasb and b is None):
        return None
    if a <= 0:
        return None
    la = (D(a) / S).ln()
    if not hasb:
        return round_units(la)
    if b <= 0 or b == S:
        return None
    lb = (D(b) / S).ln()
    y = la / lb
    # log_b(a) is rational only when a and b are powers of one base: then
    # it is exact and never a boundary here
    return round_units(y)


def exp(v):
    if v is None:
        return None
    x = D(v) / S
    if x > 12:
        return None
    return round_units(x.exp())
