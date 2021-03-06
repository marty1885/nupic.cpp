/* ---------------------------------------------------------------------
 * Numenta Platform for Intelligent Computing (NuPIC)
 * Copyright (C) 2016, Numenta, Inc.  Unless you have an agreement
 * with Numenta, Inc., for a separate license for this software code, the
 * following terms and conditions apply:
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU Affero Public License for more details.
 *
 * You should have received a copy of the GNU Affero Public License
 * along with this program.  If not, see http://www.gnu.org/licenses.
 *
 * http://numenta.org/licenses/
 * ---------------------------------------------------------------------
 */

/** @file
 * Implementation of the ScalarSensor
 */

#include <nupic/regions/ScalarSensor.hpp>

#include <nupic/engine/Input.hpp>
#include <nupic/engine/Output.hpp>
#include <nupic/engine/Region.hpp>
#include <nupic/engine/Spec.hpp>
#include <nupic/ntypes/Array.hpp>
#include <nupic/ntypes/BundleIO.hpp>
#include <nupic/utils/Log.hpp>


namespace nupic {

ScalarSensor::ScalarSensor(const ValueMap &params, Region *region)
    : RegionImpl(region) {
  const UInt32 n = params.getScalarT<UInt32>("n");
  const UInt32 w = params.getScalarT<UInt32>("w");
  const Real64 resolution = params.getScalarT<Real64>("resolution");
  const Real64 radius = params.getScalarT<Real64>("radius");
  const Real64 minValue = params.getScalarT<Real64>("minValue");
  const Real64 maxValue = params.getScalarT<Real64>("maxValue");
  const bool periodic = params.getScalarT<bool>("periodic");
  const bool clipInput = params.getScalarT<bool>("clipInput");
  if (periodic) {
    encoder_ =
        new PeriodicScalarEncoder(w, minValue, maxValue, n, radius, resolution);
  } else {
    encoder_ = new ScalarEncoder(w, minValue, maxValue, n, radius, resolution,
                                 clipInput);
  }

  sensedValue_ = params.getScalarT<Real64>("sensedValue");
}

ScalarSensor::ScalarSensor(BundleIO &bundle, Region *region)
    : RegionImpl(region) {
  deserialize(bundle);
}

  void ScalarSensor::compute()
  {
    Real32* array = (Real32*)encodedOutput_->getData().getBuffer();
    UInt *uintArray = new UInt[encoder_->getOutputWidth()];
	try {
	    const Int32 iBucket = encoder_->encodeIntoArray((Real)sensedValue_, uintArray);
	    ((Int32*)bucketOutput_->getData().getBuffer())[0] = iBucket;
	    for(UInt i=0; i<encoder_->getOutputWidth(); i++) //FIXME optimize
	    {
	      array[i] = (Real32)uintArray[i]; // copy values back to SP's 'array' array
	    }
	}
	catch(Exception& e) {
    	delete[] uintArray;
		throw e;
	}
    delete[] uintArray;
  }

ScalarSensor::~ScalarSensor() { delete encoder_; }

/* static */ Spec *ScalarSensor::createSpec() {
  auto ns = new Spec;

  ns->singleNodeOnly = true;

  /* ----- parameters ----- */
  ns->parameters.add("sensedValue",
                     ParameterSpec("Scalar input", NTA_BasicType_Real64,
                                   1,    // elementCount
                                   "",   // constraints
                                   "-1", // defaultValue
                                   ParameterSpec::ReadWriteAccess));

  ns->parameters.add("n", ParameterSpec("The length of the encoding",
                                        NTA_BasicType_UInt32,
                                        1,   // elementCount
                                        "",  // constraints
                                        "0", // defaultValue
                                        ParameterSpec::ReadWriteAccess));

  ns->parameters.add("w",
                     ParameterSpec("The number of active bits in the encoding",
                                   NTA_BasicType_UInt32,
                                   1,   // elementCount
                                   "",  // constraints
                                   "0", // defaultValue
                                   ParameterSpec::ReadWriteAccess));

  ns->parameters.add("resolution",
                     ParameterSpec("The resolution for the encoder",
                                   NTA_BasicType_Real64,
                                   1,   // elementCount
                                   "",  // constraints
                                   "0", // defaultValue
                                   ParameterSpec::ReadWriteAccess));

  ns->parameters.add("radius", ParameterSpec("The radius for the encoder",
                                             NTA_BasicType_Real64,
                                             1,   // elementCount
                                             "",  // constraints
                                             "0", // defaultValue
                                             ParameterSpec::ReadWriteAccess));

  ns->parameters.add("minValue",
                     ParameterSpec("The minimum value for the input",
                                   NTA_BasicType_Real64,
                                   1,    // elementCount
                                   "",   // constraints
                                   "-1", // defaultValue
                                   ParameterSpec::ReadWriteAccess));

  ns->parameters.add("maxValue",
                     ParameterSpec("The maximum value for the input",
                                   NTA_BasicType_Real64,
                                   1,    // elementCount
                                   "",   // constraints
                                   "-1", // defaultValue
                                   ParameterSpec::ReadWriteAccess));

  ns->parameters.add("periodic",
                     ParameterSpec("Whether the encoder is periodic",
                                   NTA_BasicType_Bool,
                                   1,       // elementCount
                                   "",      // constraints
                                   "false", // defaultValue
                                   ParameterSpec::ReadWriteAccess));

  ns->parameters.add("clipInput",
                    ParameterSpec(
                                  "Whether to clip inputs if they're outside [minValue, maxValue]",
                                  NTA_BasicType_Bool,
                                  1,       // elementCount
                                  "",      // constraints
                                  "false", // defaultValue
                                  ParameterSpec::ReadWriteAccess));

  /* ----- outputs ----- */

  ns->outputs.add("encoded", OutputSpec("Encoded value", NTA_BasicType_UInt32,
                                        0,    // elementCount
                                        true, // isRegionLevel
                                        true  // isDefaultOutput
                                        ));

  ns->outputs.add("bucket", OutputSpec("Bucket number for this sensedValue",
                                       NTA_BasicType_Int32,
                                       0,    // elementCount
                                       true, // isRegionLevel
                                       false // isDefaultOutput
                                       ));

  return ns;
}

Real64 ScalarSensor::getParameterReal64(const std::string &name, Int64 index) {
  if (name == "sensedValue") {
    return sensedValue_;
  }
  else {
    return RegionImpl::getParameterReal64(name, index);
  }
}

UInt32 ScalarSensor::getParameterUInt32(const std::string &name, Int64 index) {
  if (name == "n") {
    return (UInt32)encoder_->getOutputWidth();
  }
  else {
    return RegionImpl::getParameterUInt32(name, index);
  }
}

void ScalarSensor::setParameterReal64(const std::string &name, Int64 index, Real64 value) {
  if (name == "sensedValue") {
    sensedValue_ = value;
  } else {
	RegionImpl::setParameterReal64(name, index, value);
  }
}



void ScalarSensor::initialize() {
  encodedOutput_ = getOutput("encoded");
  bucketOutput_ = getOutput("bucket");
}

size_t ScalarSensor::getNodeOutputElementCount(const std::string &outputName) {
  if (outputName == "encoded") {
    return encoder_->getOutputWidth();
  } else if (outputName == "bucket") {
    return 1;
  } else {
    NTA_THROW << "ScalarSensor::getOutputSize -- unknown output " << outputName;
  }
}

std::string ScalarSensor::executeCommand(const std::vector<std::string> &args,
                                         Int64 index) {
  NTA_THROW << "ScalarSensor::executeCommand -- commands not supported";
}

void ScalarSensor::serialize(BundleIO &bundle) {
  NTA_THROW << "ScalarSensor::serialize -- Not implemented";
}

void ScalarSensor::deserialize(BundleIO &bundle) {
  NTA_THROW << "ScalarSensor::deserialize -- Not implemented";
}


} // namespace nupic
