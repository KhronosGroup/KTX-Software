# Copyright (c) 2023, Shukant Pal and Contributors
# SPDX-License-Identifier: Apache-2.0

# Configuration file for the Sphinx documentation builder.
#
# For the full list of built-in configuration values, see the documentation:
# https://www.sphinx-doc.org/en/master/usage/configuration.html

# -- Project information -----------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#project-information

import os
import sys

# NOTA BENE
# The compiled modules that autodoc is trying to load are in
# <ktx-source>/interface/python_binding but when autodoc is run it is
# given the build directory as its "source" directory so it uses the copy
# of this file in the build directory. Therefore the following 3 lines add
# the build directory to sys.path which does not help autodoc find the
# modules. However something is adding <ktx-source>/interface/python_binding
# to sys.path. My best guess is that sphinx/autodoc add its working
# directory which is the source directory.
current_dir = os.path.dirname(__file__)
target_dir = os.path.abspath(os.path.join(current_dir, "."))
sys.path.insert(0, target_dir)

#print("*******" + __file__ + "**********\n", file=sys.stderr)
#print(sys.path, file=sys.stderr)

project = 'pyktx'
copyright = '2025, Khronos Group, Inc. 2023, Shukant Pal'
author = 'Shukant Pal, Mark Callow'

# -- General configuration ---------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#general-configuration

extensions = [
    'sphinx.ext.autodoc',
    'sphinx.ext.viewcode',
    'sphinx.ext.napoleon',
]

templates_path = ['_templates']
exclude_patterns = ['_build', 'Thumbs.db', '.DS_Store']



# -- Options for HTML output -------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#options-for-html-output

html_theme = 'sphinx_rtd_theme'
html_static_path = ['_static']
