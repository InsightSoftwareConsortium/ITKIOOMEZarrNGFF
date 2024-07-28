/*=========================================================================
 *
 *  Copyright NumFOCUS
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *         https://www.apache.org/licenses/LICENSE-2.0.txt
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *=========================================================================*/
#ifndef itkOMEZarrNGFFTransformIOFactory_h
#define itkOMEZarrNGFFTransformIOFactory_h
#include "IOOMEZarrNGFFExport.h"

#include "itkObjectFactoryBase.h"
#include "itkTransformIOBase.h"

namespace itk
{
/** \class OMEZarrNGFFTransformIOFactory
 * \brief Create instances of OMEZarrNGFFTransformIO objects using an object factory.
 * \ingroup ITKIOOMEZarrNGFF
 */
class IOOMEZarrNGFF_EXPORT OMEZarrNGFFTransformIOFactory : public ObjectFactoryBase
{
public:
  /** Standard class typedefs. */
  using Self = OMEZarrNGFFTransformIOFactory;
  using Superclass = ObjectFactoryBase;
  using Pointer = SmartPointer<Self>;
  using ConstPointer = SmartPointer<const Self>;

  /** Class methods used to interface with the registered factories. */
  const char *
  GetITKSourceVersion() const override;

  const char *
  GetDescription() const override;

  /** Method for class instantiation. */
  itkFactorylessNewMacro(Self);

  /** Run-time type information (and related methods). */
  itkTypeMacro(OMEZarrNGFFTransformIOFactory, ObjectFactoryBase);

  /** Register one factory of this type  */
  static void
  RegisterOneFactory()
  {
    OMEZarrNGFFTransformIOFactory::Pointer zarrFactory = OMEZarrNGFFTransformIOFactory::New();

    ObjectFactoryBase::RegisterFactoryInternal(zarrFactory);
  }

protected:
  OMEZarrNGFFTransformIOFactory();
  ~OMEZarrNGFFTransformIOFactory() override = default;

private:
  ITK_DISALLOW_COPY_AND_MOVE(OMEZarrNGFFTransformIOFactory);
};
} // end namespace itk

#endif
