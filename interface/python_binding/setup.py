from setuptools import setup

__name__    = 'pyktx'
__version__ = '4.1.0rc3'

setup(
    name=__name__,
    version=__version__,
    description='A Python interface to the libktx library',
    author='Shukant Pal',
    author_email='foss@shukantpal.com',
    cffi_modules=["build.py:ffibuilder"],
    classifiers=[
        "Programming Language :: Python :: 3",
    ],
    include_package_data=True,
    install_requires=["cffi>=1.15.1"],
    packages=['pyktx'],
    package_dir={'pyktx': 'pyktx'},
    setup_requires=["cffi>=1.15.1"],
    url='https://github.com/KhronosGroup/KTX-Software'
)
