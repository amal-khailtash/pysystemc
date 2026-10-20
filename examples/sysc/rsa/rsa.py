"""Port Accellera sysc/rsa/rsa.cpp (Apache-2.0; upstream LICENSE/NOTICE).

Native 250-bit assignments preserve the source's arithmetic width boundaries.
Linux drand48 preserves the seeded source random stream. This example is a
reference model, not a cryptographic library. Nonpositive seeds use wall time.
"""

import ctypes
import sys
import time
from amal.eda.systemc import sc_dt as dt

BIG = dt.sc_bigint[250]
LIBC = ctypes.CDLL(None)
LIBC.drand48.restype = ctypes.c_double
LIBC.srand48.argtypes = [ctypes.c_long]


def native(value):
    """Assign an integer through native signed 250-bit storage and return it."""
    return int(str(BIG(str(value)).to_string(dt.SC_DEC)))


def remainder(a, b):
    """Return the C++ signed remainder using division truncated toward zero."""
    quotient = abs(a) // abs(b)
    if (a < 0) != (b < 0):
        quotient = -quotient
    return a - quotient * b


def random_bits(size):
    """Generate the upstream positive binary string with its sign/prefix sizing."""
    return int("0" + "".join("1" if LIBC.drand48() < 0.5 else "0" for _ in range(3, size)), 2)


def modular_exp(a, b, n):
    """Compute source square/multiply order with native assignment boundaries."""
    value = 1
    for bit in range(249, -1, -1):
        value = native(remainder(value * value, n))
        if b & (1 << bit):
            value = native(remainder(value * a, n))
    return value + n if value < 0 else value


def witness(a, n):
    """Apply the source Miller-Rabin witness loop over all 250 native bits."""
    value = 1
    for bit in range(249, -1, -1):
        old = abs(value)
        value = native(remainder(value * value, n))
        if abs(value) == 1 and old not in (1, n - 1):
            return True
        if (n - 1) & (1 << bit):
            value = native(remainder(value * a, n))
    return abs(value) != 1


def probable_prime(n):
    """Apply source small-divisor screening and fifty randomized witnesses."""
    if n <= 2:
        return False
    limit = n - 2 if n < 1023 else 1023
    if any(n % divisor == 0 for divisor in range(3, limit + 1, 2)):
        return False
    for _ in range(50):
        a = random_bits(252) % (n - 1) + 1
        if witness(a, n):
            return False
    return True


def find_prime(mask):
    """Choose a random odd candidate and advance through source modulo range."""
    candidate = random_bits(127) | 1
    while not probable_prime(candidate):
        candidate = native((candidate + 2) % mask)
    return candidate


def euclid(a, b):
    """Return source extended Euclid gcd and native-width coefficient assignments."""
    if not b:
        return a, 1, 0
    divisor, x, y = euclid(b, remainder(a, b))
    quotient = abs(a) // abs(b) * (-1 if (a < 0) != (b < 0) else 1)
    return divisor, y, native(x - quotient * y)


def run(seed):
    """Generate keys and verify the upstream seeded RSA round trip.

    Args:
        seed: Positive drand48 seed; nonpositive values select current wall time.
    Returns:
        None after printing keys and verifying message recovery.
    """
    seed = seed if seed > 0 else int(time.time())
    LIBC.srand48(seed)
    print(f"\nRandom number generator seed = {seed}\n")
    mask = (1 << 124) - 1
    p, q = find_prime(mask), find_prime(mask)
    n = native(p * q)
    m = native((p - 1) * (q - 1))
    e = 3
    while euclid(e, m)[0] != 1:
        e += 2
    divisor, x, _ = euclid(e, m)
    assert divisor == 1
    d = remainder(x, m)
    if d < 0:
        d += m
    print(f"RSA public key P: P=( e, n )\ne = {e}\nn = {n}\n")
    print(f"RSA secret key S: S=( d, n )\nd = {d}\nn = {n}\n")
    message = random_bits(252) % n
    print(f"Message to be ciphered = \n{message}")
    encrypted = modular_exp(message, e, n)
    print(f"\nCiphered message = \n{encrypted}")
    recovered = modular_exp(encrypted, d, n)
    print(f"\nDeciphered message = \n{recovered}")
    assert recovered == message and message > 0
    print("\nNote that the original message == the deciphered message, ")
    print("showing that this algorithm and implementation work correctly.\n")


if __name__ == "__main__":
    run(int(sys.argv[1]) if len(sys.argv) > 1 else -1)
