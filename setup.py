import setuptools

setuptools.setup(
    name='pyfzf_launcher',
    version='1.0',
    description='Python wrapper to launch apps defined on .desktop files',
    py_modules=['pyfzf_launcher'],
    entry_points={
        'console_scripts': [
            'pyfzf-launcher=pyfzf_launcher:main',
        ],
    }
)
