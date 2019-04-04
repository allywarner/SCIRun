/*
   For more information, please see: http://software.sci.utah.edu

   The MIT License

   Copyright (c) 2016 Scientific Computing and Imaging Institute,
   University of Utah.

   License for the specific language governing rights and limitations under
   Permission is hereby granted, free of charge, to any person obtaining a
   copy of this software and associated documentation files (the "Software"),
   to deal in the Software without restriction, including without limitation
   the rights to use, copy, modify, merge, publish, distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the
   Software is furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included
   in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
   OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
   THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
   DEALINGS IN THE SOFTWARE.
*/

#include <Modules/Python/InterfaceWithPython.h>
#include <Modules/Python/PythonObjectForwarder.h>
#ifdef BUILD_WITH_PYTHON
#include <Core/Python/PythonInterpreter.h>
#include <Core/Logging/Log.h>
// ReSharper disable once CppUnusedIncludeDirective
#include <Core/Datatypes/Legacy/Field/Field.h>
#include <boost/algorithm/string.hpp>
#include <Dataflow/Engine/Python/NetworkEditorPythonAPI.h>
#endif

using namespace SCIRun::Modules::Python;
using namespace SCIRun::Core::Datatypes;
using namespace SCIRun::Dataflow::Networks;
using namespace SCIRun::Core;
using namespace SCIRun::Core::Thread;
using namespace SCIRun::Core::Algorithms;
using namespace SCIRun::Core::Algorithms::Python;

ALGORITHM_PARAMETER_DEF(Python, PythonCode);
ALGORITHM_PARAMETER_DEF(Python, PythonTopLevelCode);
ALGORITHM_PARAMETER_DEF(Python, PythonInputStringNames);
ALGORITHM_PARAMETER_DEF(Python, PythonInputMatrixNames);
ALGORITHM_PARAMETER_DEF(Python, PythonInputFieldNames);
ALGORITHM_PARAMETER_DEF(Python, PythonOutputString1Name);
ALGORITHM_PARAMETER_DEF(Python, PythonOutputString2Name);
ALGORITHM_PARAMETER_DEF(Python, PythonOutputString3Name);
ALGORITHM_PARAMETER_DEF(Python, PythonOutputMatrix1Name);
ALGORITHM_PARAMETER_DEF(Python, PythonOutputMatrix2Name);
ALGORITHM_PARAMETER_DEF(Python, PythonOutputMatrix3Name);
ALGORITHM_PARAMETER_DEF(Python, PythonOutputField1Name);
ALGORITHM_PARAMETER_DEF(Python, PythonOutputField2Name);
ALGORITHM_PARAMETER_DEF(Python, PythonOutputField3Name);

MODULE_INFO_DEF(InterfaceWithPython, Python, SCIRun)

Mutex InterfaceWithPython::lock_("InterfaceWithPython");

InterfaceWithPython::InterfaceWithPython() : Module(staticInfo_)
{
  INITIALIZE_PORT(InputMatrix);
  INITIALIZE_PORT(InputField);
  INITIALIZE_PORT(InputString);
  INITIALIZE_PORT(PythonMatrix1);
  INITIALIZE_PORT(PythonField1);
  INITIALIZE_PORT(PythonString1);
  INITIALIZE_PORT(PythonMatrix2);
  INITIALIZE_PORT(PythonField2);
  INITIALIZE_PORT(PythonString2);
  INITIALIZE_PORT(PythonMatrix3);
  INITIALIZE_PORT(PythonField3);
  INITIALIZE_PORT(PythonString3);
}

void InterfaceWithPython::setStateDefaults()
{
  auto state = get_state();

  state->setValue(Parameters::PythonCode, std::string("# Insert your Python code here. The SCIRun API package is automatically imported."));
  state->setValue(Parameters::PythonTopLevelCode,
    std::string("# Main namespace/top level context code goes here; for example, import statements and global variables.\n"
    "# This code will be executed before the 'Code' tab, and no input/output variables are available."));
  state->setValue(Parameters::PollingIntervalMilliseconds, 200);
  state->setValue(Parameters::NumberOfRetries, 50);

  state->setValue(Parameters::PythonOutputField1Name, std::string("fieldOutput1"));
  state->setValue(Parameters::PythonOutputField2Name, std::string("fieldOutput2"));
  state->setValue(Parameters::PythonOutputField3Name, std::string("fieldOutput3"));

  state->setValue(Parameters::PythonOutputString1Name, std::string("stringOutput1"));
  state->setValue(Parameters::PythonOutputString2Name, std::string("stringOutput2"));
  state->setValue(Parameters::PythonOutputString3Name, std::string("stringOutput3"));

  state->setValue(Parameters::PythonOutputMatrix1Name, std::string("matrixOutput1"));
  state->setValue(Parameters::PythonOutputMatrix2Name, std::string("matrixOutput2"));
  state->setValue(Parameters::PythonOutputMatrix3Name, std::string("matrixOutput3"));
}

std::vector<AlgorithmParameterName> InterfaceWithPython::outputNameParameters()
{
  return { Parameters::PythonOutputMatrix1Name, Parameters::PythonOutputMatrix2Name, Parameters::PythonOutputMatrix3Name,
    Parameters::PythonOutputField1Name, Parameters::PythonOutputField2Name, Parameters::PythonOutputField3Name,
    Parameters::PythonOutputString1Name, Parameters::PythonOutputString2Name, Parameters::PythonOutputString3Name };
}

std::string InterfaceWithPython::convertOutputSyntax(const std::string& code) const
{
  auto outputVarsToCheck = outputNameParameters();

  for (const auto& var : outputVarsToCheck)
  {
    auto varName = cstate()->getValue(var).toString();

    auto regexString = "(\\h*)" + varName + " = (.+)";
    //std::cout << "REGEX STRING " << regexString << std::endl;
    boost::regex outputRegex(regexString);
    boost::smatch what;
    if (regex_match(code, what, outputRegex))
    {
      int rhsIndex = what.size() > 2 ? 2 : 1;
      auto whitespace = what.size() > 2 ? boost::lexical_cast<std::string>(what[1]) : "";
      auto rhs = boost::lexical_cast<std::string>(what[rhsIndex]);
      auto converted = whitespace + "scirun_set_module_transient_state(\"" + get_id().id_ + "\",\"" + varName + "\"," + rhs + ")";
      //std::cout << "CONVERTED TO " << converted << std::endl;
      return converted;
    }
  }

  return code;
}

std::string InterfaceWithPython::convertInputSyntax(const std::string& code) const
{
  for (const auto& port : inputPorts())
  {
    if (port->nconnections() > 0)
    {
      auto inputName = cstate()->getValue(Name(port->id().toString())).toString();
      //std::cout << "FOUND INPUT VARIABLE NAME: " << inputName << " for port " << port->id().toString() << std::endl;
      //std::cout << "NEED TO REPLACE " << inputName << " with\n\t" << "scirun_get_module_input_value(\"" << get_id() << "\", \"" << port->id().toString() << "\")" << std::endl;
      auto index = code.find(inputName);
      if (index != std::string::npos)
      {
        auto codeCopy = code;
        return codeCopy.replace(index, inputName.length(), "scirun_get_module_input_value(\"" + get_id().id_ + "\", \"" + port->id().toString() + "\")");
      }
    }
  }
  return code;
}

void InterfaceWithPython::execute()
{
#ifdef BUILD_WITH_PYTHON
  auto matrices = getOptionalDynamicInputs(InputMatrix);
  auto fields = getOptionalDynamicInputs(InputField);
  auto strings = getOptionalDynamicInputs(InputString);
  if (needToExecute())
  {
    auto state = get_state();
    {
      Guard g(lock_.get());

      runTopLevelCode();

      auto code = state->getValue(Parameters::PythonCode).toString();
      extractSpecialBlocks(code);

      std::ostringstream convertedCode;
      std::vector<std::string> lines;
      boost::split(lines, code, boost::is_any_of("\n"));
      for (const auto& line : lines)
      {
        convertedCode << convertInputSyntax(convertOutputSyntax(line)) << "\n";
      }

      NetworkEditorPythonAPI::PythonModuleContextApiDisabler disabler;
      PythonInterpreter::Instance().run_script(convertedCode.str());
    }

    PythonObjectForwarderImpl<InterfaceWithPython> impl(*this);

    if (oport_connected(PythonString1))
      impl.waitForOutputFromTransientState(state->getValue(Parameters::PythonOutputString1Name).toString(), PythonString1, PythonMatrix1, PythonField1);
    if (oport_connected(PythonString2))
      impl.waitForOutputFromTransientState(state->getValue(Parameters::PythonOutputString2Name).toString(), PythonString2, PythonMatrix1, PythonField1);
    if (oport_connected(PythonString3))
      impl.waitForOutputFromTransientState(state->getValue(Parameters::PythonOutputString3Name).toString(), PythonString3, PythonMatrix1, PythonField1);
    if (oport_connected(PythonMatrix1))
      impl.waitForOutputFromTransientState(state->getValue(Parameters::PythonOutputMatrix1Name).toString(), PythonString1, PythonMatrix1, PythonField1);
    if (oport_connected(PythonMatrix2))
      impl.waitForOutputFromTransientState(state->getValue(Parameters::PythonOutputMatrix2Name).toString(), PythonString1, PythonMatrix2, PythonField1);
    if (oport_connected(PythonMatrix3))
      impl.waitForOutputFromTransientState(state->getValue(Parameters::PythonOutputMatrix3Name).toString(), PythonString1, PythonMatrix3, PythonField1);
    if (oport_connected(PythonField1))
      impl.waitForOutputFromTransientState(state->getValue(Parameters::PythonOutputField1Name).toString(), PythonString1, PythonMatrix1, PythonField1);
    if (oport_connected(PythonField2))
      impl.waitForOutputFromTransientState(state->getValue(Parameters::PythonOutputField2Name).toString(), PythonString1, PythonMatrix1, PythonField2);
    if (oport_connected(PythonField3))
      impl.waitForOutputFromTransientState(state->getValue(Parameters::PythonOutputField3Name).toString(), PythonString1, PythonMatrix1, PythonField3);
  }
#else
  error("This module does nothing, turn on BUILD_WITH_PYTHON to enable.");
#endif
}

void InterfaceWithPython::runTopLevelCode() const
{
#ifdef BUILD_WITH_PYTHON
  auto topLevelCode = cstate()->getValue(Parameters::PythonTopLevelCode).toString();
  std::vector<std::string> lines;
  boost::split(lines, topLevelCode, boost::is_any_of("\n"));
  for (const auto& line : lines)
  {
    PythonInterpreter::Instance().run_string(line);
  }
#endif
}

void InterfaceWithPython::extractSpecialBlocks(const std::string& code) const
{
  logCritical("Code: {}", code);
  static boost::regex matlabBlock("(.*)\\/\\*matlab(.*)matlab\\*\\/(.*)");

  std::string::const_iterator start, end;
  start = code.begin();
  end = code.end();
  boost::match_results<std::string::const_iterator> what;
  boost::match_flag_type flags = boost::match_default;
  while (regex_search(start, end, what, matlabBlock, flags))
  {
    // what[0] contains the whole string
    // what[5] contains the class name.
    // what[6] contains the template specialisation if any.
    // add class name and position to map:
    // m[std::string(what[5].first, what[5].second)
    //       + std::string(what[6].first, what[6].second)]
    //    = what[5].first - file.begin();
    auto firstPart = std::string(what[1]);
    auto matlabPart = std::string(what[2]);
    auto secondPart = std::string(what[3]);
    // update search position:
    start = what[2].second;

    // update flags:
    flags |= boost::match_prev_avail;
    flags |= boost::match_not_bob;

    logCritical("First: {}", firstPart);
    logCritical("Matlab: {}", matlabPart);
    logCritical("Second: {}", secondPart);

    logCritical("Next search string: {}", std::string(start, end));

  }

}
