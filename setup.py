#!/usr/bin/env python

"""
setup.py file for hl_vt100
"""

from distutils.core import setup, Extension


hl_vt100_module = Extension('_hl_vt100',
                           sources=['hl_vt100_wrap.c',
                                    'src/hl_vt100.c',
                                    'src/lw_terminal_parser.c',
                                    'src/lw_terminal_vt100.c'],
                           )

setup (name = 'hl_vt100',
       version = '0.1',
       author      = "Julien Palard",
       description = """Headless vt100 emulator""",
       ext_modules = [hl_vt100_module],
       py_modules = ["hl_vt100"],
       )
