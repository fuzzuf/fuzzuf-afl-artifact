#!/usr/bin/env python3
import site
print (site.getsitepackages()[-1].rstrip(),end="")

