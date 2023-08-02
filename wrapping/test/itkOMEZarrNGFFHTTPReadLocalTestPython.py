#==========================================================================
#
#   Copyright NumFOCUS
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#          https://www.apache.org/licenses/LICENSE-2.0.txt
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.
#
#==========================================================================*/

# Test reading OME-Zarr NGFF chunked data over HTTP.
#
# This test spawns a Python web server child process to serve from the
# test data directory on port 9999, then reads the local OME-Zarr file
# as if it were remotely served.

import os
import subprocess
import sys

import itk
import numpy as np

TEST_PORT = 9999
LOCALHOST_BINDING = '127.0.0.1'

itk.auto_progress(2)

imageio = itk.OMEZarrNGFFImageIO.New()

if(len(sys.argv) < 2):
    raise ValueError('Expected arguments: <path/to/input.mha> <path/to/output.zarr>')

# Test setup: create OME-Zarr store on local disk
print(f"Reading {sys.argv[1]}")
image = itk.imread(sys.argv[1], pixel_type=itk.F)

# Assign arbitrary spacing and origin to assess metadata I/O
# Note that direction is not supported under the OME-Zarr v0.4 spec
image.SetSpacing(np.arange(0.2, 0.1 * image.GetImageDimension() + 0.19, 0.1))
image.SetOrigin(np.arange(0.6, 0.1 * image.GetImageDimension() + 0.59, 0.1))

print(f"Writing {sys.argv[2]}")
itk.imwrite(image, sys.argv[2], imageio=imageio, compression=False)

# Serve files on "localhost" in the background
p = subprocess.Popen([sys.executable,
                      '-m', 'http.server', str(TEST_PORT),
                      '--directory', os.path.dirname(sys.argv[2]),
                      '--bind', LOCALHOST_BINDING])

url = f'http://localhost:{TEST_PORT}/{os.path.basename(sys.argv[2])}'

print(f"Reading {url}")
image2 = itk.imread(url, imageio=imageio)
print(image2)

# Compare metadata
assert np.all(np.array(itk.size(image2)) == np.array(itk.size(image))), 'Image size mismatch'
assert np.all(np.array(itk.spacing(image2)) == np.array(itk.spacing(image))), 'Image spacing mismatch'
assert np.all(np.array(itk.origin(image2)) == np.array(itk.origin(image))), 'Image origin mismatch'

# Compare data
assert np.all(itk.array_view_from_image(image2) == itk.array_view_from_image(image)), 'Image data mismatch'

# Clean up
p.kill()
