from setuptools import setup

setup(
    name='pyktx',
    version='4.0.0-RC14',
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
