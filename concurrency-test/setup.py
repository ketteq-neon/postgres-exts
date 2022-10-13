from setuptools import setup
from xsct.version import xsct_version

setup(
    name='xsct',
    version=xsct_version,
    description='KetteQ Extensions Concurrency Test [xsct]',
    url='https://ketteq.com/',
    author='Giancarlo A. Chiappe',
    author_email='giancarlo.chiappe@ketteq.com',
    license='Proprietary',
    packages=['xsct'],
    install_requires=[],
    classifiers=[
        'Development Status :: 4 - Beta',
        'Intended Audience :: Developers',
        'License :: Other/Proprietary License',
        'Operating System :: OS Independent',
        'Programming Language :: Python :: 3',
    ],
    entry_points="""
        [console_scripts]
        xsct = xsct.suite:main
        """,
)
