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
# This test accepts a remote OME-Zarr endpoint and reads
# its data over HTTP/HTTPS.

import sys

import itk

itk.auto_progress(2)

if(len(sys.argv) < 2):
    raise ValueError('Expected arguments: <zarr_url> [datasetIndex]')

assert sys.argv[1].startswith('http'), 'OME-Zarr remote store location must be an HTTP endpoint'
assert sys.argv[1].endswith('.zarr'), 'Expected `.zarr` store remote location'

imageio = None
if len(sys.argv) > 2:
    dataset_index = int(sys.argv[2])
    imageio = itk.OMEZarrNGFFImageIO.New(dataset_index=dataset_index)

# Read dataset from remote server
# TODO: Add support for 4D images and multichannel images
# https://github.com/InsightSoftwareConsortium/ITKIOOMEZarrNGFF/issues/32
print(f"Reading {sys.argv[1]}")
image = itk.imread(sys.argv[1], pixel_type=itk.F, imageio=imageio)

print(f'Read in image:')
print(image)
