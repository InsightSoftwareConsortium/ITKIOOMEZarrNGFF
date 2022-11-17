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


import itk
import sys

itk.auto_progress(2)

imageio = itk.OMEZarrNGFFImageIO.New()

print(f"Reading {sys.argv[1]}")
image = itk.imread(sys.argv[1])

print(f"Writing {sys.argv[2]}")
itk.imwrite(image, sys.argv[2], imageio=imageio, compression=False)

print(f"Reading {sys.argv[2]}")
image2 = itk.imread(sys.argv[2], imageio=imageio)

print(f"Writing {sys.argv[3]}")
itk.imwrite(image, sys.argv[3])

print("Test finished")
