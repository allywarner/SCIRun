/*
   For more information, please see: http://software.sci.utah.edu

   The MIT License

   Copyright (c) 2012 Scientific Computing and Imaging Institute,
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


#ifndef DATAFLOW_NETWORK_MODULE_H
#define DATAFLOW_NETWORK_MODULE_H 

#include <boost/noncopyable.hpp>
#include <boost/static_assert.hpp>
#include <boost/lexical_cast.hpp>
#include <vector>
#include <Core/Logging/LoggerInterface.h>
#include <Core/Datatypes/DatatypeFwd.h>
#include <Core/Datatypes/Mesh/FieldFwd.h>
#include <Core/Algorithms/Base/AlgorithmFwd.h>
#include <Dataflow/Network/NetworkFwd.h>
#include <Dataflow/Network/ModuleInterface.h>
#include <Dataflow/Network/ModuleStateInterface.h>
#include <Dataflow/Network/ModuleDescription.h>
#include <Dataflow/Network/PortManager.h>
#include <Dataflow/Network/share.h>

namespace SCIRun {
namespace Dataflow {
namespace Networks {
  
  class SCISHARE Module : public ModuleInterface, public Core::Logging::LoggerInterface, boost::noncopyable
  {
  public:
    Module(const ModuleLookupInfo& info, 
      bool hasUi = true, 
      Core::Algorithms::AlgorithmFactoryHandle algoFactory = defaultAlgoFactory_,
      ModuleStateFactoryHandle stateFactory = defaultStateFactory_,
      const std::string& version = "1.0");
    virtual ~Module();

    std::string get_module_name() const { return info_.module_name_; }
    std::string get_categoryname() const { return info_.category_name_; }
    std::string get_packagename() const { return info_.package_name_; }
    ModuleId get_id() const { return id_; }

    //for serialization
    virtual const ModuleLookupInfo& get_info() const { return info_; }
    virtual void set_id(const std::string& id) { id_ = ModuleId(id); }
  
    //for unit testing. Need to restrict access somehow.
    static void resetInstanceCount() { instanceCount_ = 0; }

    bool has_ui() const { return has_ui_; }
    void setUiVisible(bool visible); 
    virtual size_t num_input_ports() const;
    virtual size_t num_output_ports() const;

    virtual bool hasInputPort(const PortId& id) const;
    virtual bool hasOutputPort(const PortId& id) const;
    virtual InputPortHandle getInputPort(const PortId& id) const;
    virtual OutputPortHandle getOutputPort(const PortId& id) const;
    virtual std::vector<InputPortHandle> findInputPortsWithName(const std::string& name) const;
    virtual std::vector<OutputPortHandle> findOutputPortsWithName(const std::string& name) const;
    virtual std::vector<InputPortHandle> inputPorts() const;
    virtual std::vector<OutputPortHandle> outputPorts() const;

    //TODO: execute signal here.
    virtual void do_execute() throw(); //--C++11--will throw nothing
    virtual ModuleStateHandle get_state();
    virtual void set_state(ModuleStateHandle state);

  private:
    virtual SCIRun::Core::Datatypes::DatatypeHandleOption get_input_handle(const PortId& id);
    virtual std::vector<SCIRun::Core::Datatypes::DatatypeHandleOption> get_dynamic_input_handles(const PortId& id);
    virtual void send_output_handle(const PortId& id, SCIRun::Core::Datatypes::DatatypeHandle data);

  public:
    virtual void setLogger(SCIRun::Core::Logging::LoggerHandle log);
    virtual SCIRun::Core::Logging::LoggerHandle getLogger() const;
    virtual void error(const std::string& msg) const { errorSignal_(id_); getLogger()->error(msg); }
    virtual void warning(const std::string& msg) const { getLogger()->warning(msg); }
    virtual void remark(const std::string& msg) const { getLogger()->remark(msg); }
    virtual void status(const std::string& msg) const { getLogger()->status(msg); }

    virtual SCIRun::Core::Algorithms::AlgorithmStatusReporter::UpdaterFunc getUpdaterFunc() const { return updaterFunc_; }
    virtual void setUpdaterFunc(SCIRun::Core::Algorithms::AlgorithmStatusReporter::UpdaterFunc func);
    virtual void setUiToggleFunc(UiToggleFunc func) { uiToggleFunc_ = func; }

    virtual boost::signals2::connection connectExecuteBegins(const ExecuteBeginsSignalType::slot_type& subscriber);
    virtual boost::signals2::connection connectExecuteEnds(const ExecuteEndsSignalType::slot_type& subscriber);
    virtual boost::signals2::connection connectErrorListener(const ErrorSignalType::slot_type& subscriber);

    virtual Core::Algorithms::AlgorithmHandle getAlgorithm() const { return algo_; }

    virtual bool needToExecute() const  
    {
      return true; //TODO
    }

    virtual bool hasDynamicPorts() const 
    {
      return false; //TODO: need to examine HasPorts base classes
    }

    bool oport_connected(const PortId& id) const;

    template <class Type, size_t N>
    struct PortNameBase
    {
      explicit PortNameBase(const PortId& id) : id_(id) {}
      //operator size_t() const { return N; }

      operator PortId() const 
      {
        return toId();
      }

      PortId toId() const
      { 
        if (id_.name.empty())
          BOOST_THROW_EXCEPTION(DataPortException() << SCIRun::Core::ErrorMessage("Port name not initialized!"));
        return id_; 
      }
      operator std::string() const
      {
        return toId().name;
      }

      PortId id_;
    };
    
    template <class Type, size_t N>
    struct StaticPortName : PortNameBase<Type,N>
    {
      explicit StaticPortName(const PortId& id = PortId(0, "[not defined yet]")) : PortNameBase<Type,N>(id) {}
    };

    template <class Type, size_t N>
    struct DynamicPortName : PortNameBase<Type,N>
    {
      explicit DynamicPortName(const PortId& id = PortId(0, "[not defined yet]")) : PortNameBase<Type,N>(id) {}
    };

    // Throws if input is not present or null.
    template <class T, size_t N>
    boost::shared_ptr<T> getRequiredInput(const StaticPortName<T,N>& port);

    template <class T, size_t N>
    boost::optional<boost::shared_ptr<T>> getOptionalInput(const StaticPortName<T,N>& port);

    template <class T, size_t N>
    std::vector<boost::shared_ptr<T>> getRequiredDynamicInputs(const DynamicPortName<T,N>& port);

    template <class T, class D, size_t N>
    void sendOutput(const StaticPortName<T,N>& port, boost::shared_ptr<D> data);

    template <class T, size_t N>
    void sendOutputFromAlgorithm(const StaticPortName<T,N>& port, const Core::Algorithms::AlgorithmOutput& output);

    class SCISHARE Builder : boost::noncopyable
    {
    public:
      Builder();
      Builder& with_name(const std::string& name);
      Builder& using_func(ModuleMaker create);
      Builder& add_input_port(const Port::ConstructionParams& params);
      Builder& add_output_port(const Port::ConstructionParams& params);
      Builder& setStateDefaults();
      ModuleHandle build();

      //TODO: these don't quite belong here, think about extracting
      PortId cloneInputPort(ModuleHandle module, const PortId& id);
      void removeInputPort(ModuleHandle module, const PortId& id);

      typedef boost::function<SCIRun::Dataflow::Networks::DatatypeSinkInterface*()> SinkMaker;
      typedef boost::function<SCIRun::Dataflow::Networks::DatatypeSourceInterface*()> SourceMaker;
      static void use_sink_type(SinkMaker func);
      static void use_source_type(SourceMaker func);
    private:
      void addInputPortImpl(Module& module, const Port::ConstructionParams& params);
      boost::shared_ptr<Module> module_;
      static SinkMaker sink_maker_;
      static SourceMaker source_maker_;
    };

    //TODO: yuck
    static ModuleStateFactoryHandle defaultStateFactory_;
    static Core::Algorithms::AlgorithmFactoryHandle defaultAlgoFactory_;

  protected:
    ModuleLookupInfo info_;
    ModuleId id_;

    Core::Algorithms::AlgorithmBase& algo();

  protected:
    enum State {
      NeedData,
      JustStarted,
      Executing,
      Completed
    };
    void update_state(State) { /*TODO*/ }

  private:
    template <class T>
    boost::shared_ptr<T> getRequiredInputAtIndex(const PortId& id);
    template <class T>
    boost::optional<boost::shared_ptr<T>> getOptionalInputAtIndex(const PortId& id);
    template <class T>
    boost::shared_ptr<T> checkInput(SCIRun::Core::Datatypes::DatatypeHandleOption inputOpt, const PortId& id);


    friend class Builder;
    size_t add_input_port(InputPortHandle);
    size_t add_output_port(OutputPortHandle);
    void removeInputPort(const PortId& id);
    bool has_ui_;

    Core::Algorithms::AlgorithmHandle algo_;
   
    ModuleStateHandle state_;
    PortManager<OutputPortHandle> oports_;
    PortManager<InputPortHandle> iports_;

    ExecuteBeginsSignalType executeBegins_;
    ExecuteEndsSignalType executeEnds_;
    ErrorSignalType errorSignal_;

    SCIRun::Core::Logging::LoggerHandle log_;
    SCIRun::Core::Algorithms::AlgorithmStatusReporter::UpdaterFunc updaterFunc_;
    UiToggleFunc uiToggleFunc_;
    static int instanceCount_;
    static SCIRun::Core::Logging::LoggerHandle defaultLogger_;
  };

  template <class T>
  boost::shared_ptr<T> Module::getRequiredInputAtIndex(const PortId& id)
  {
    auto inputOpt = get_input_handle(id);
    if (!inputOpt)
      MODULE_ERROR_WITH_TYPE(NoHandleOnPortException, "Input data required on port " + id.name);

    return checkInput<T>(inputOpt, id);
  }
  
  template <class T, size_t N>
  boost::shared_ptr<T> Module::getRequiredInput(const StaticPortName<T,N>& port)
  {
    return getRequiredInputAtIndex<T>(port.toId());
  }

  template <class T>
  boost::optional<boost::shared_ptr<T>> Module::getOptionalInputAtIndex(const PortId& id)
  {
    auto inputOpt = get_input_handle(id);
    if (!inputOpt)
      return boost::optional<boost::shared_ptr<T>>();

    return checkInput<T>(inputOpt, id);
  }

  template <class T, size_t N>
  std::vector<boost::shared_ptr<T>> Module::getRequiredDynamicInputs(const DynamicPortName<T,N>& port)
  {
    auto handleOptions = get_dynamic_input_handles(port.id_);
    std::vector<boost::shared_ptr<T>> handles;
    auto check = [&, this](SCIRun::Core::Datatypes::DatatypeHandleOption opt) { return this->checkInput<T>(opt, port.id_); };
    auto end = handleOptions.end() - 1; //leave off empty final port
    std::transform(handleOptions.begin(), end, std::back_inserter(handles), check);
    return handles;
  }

  template <class T, size_t N>
  boost::optional<boost::shared_ptr<T>> Module::getOptionalInput(const StaticPortName<T,N>& port)
  {
    return getOptionalInputAtIndex<T>(port.id_);
  }

  template <class T, class D, size_t N>
  void Module::sendOutput(const StaticPortName<T,N>& port, boost::shared_ptr<D> data)
  {
    const bool datatypeForThisPortMustBeCompatible = boost::is_base_of<T,D>::value;
    BOOST_STATIC_ASSERT(datatypeForThisPortMustBeCompatible);
    send_output_handle(port.id_, data);
  }
  
  template <class T, size_t N>
  void Module::sendOutputFromAlgorithm(const StaticPortName<T,N>& port, const Core::Algorithms::AlgorithmOutput& output)
  {
    sendOutput<T, T, N>(port, output.get<T>(Core::Algorithms::AlgorithmParameterName(port)));
  }

  template <class T>
  boost::shared_ptr<T> Module::checkInput(SCIRun::Core::Datatypes::DatatypeHandleOption inputOpt, const PortId& id)
  {
    if (!*inputOpt)
      MODULE_ERROR_WITH_TYPE(NullHandleOnPortException, "Null handle on port " + id.name);

    boost::shared_ptr<T> data = boost::dynamic_pointer_cast<T>(*inputOpt);
    if (!data)
    {
      std::ostringstream ostr;
      ostr << "Wrong datatype on port #" << id.name << "; expected " << typeid(T).name() << " but received " << typeid(*inputOpt).name();
      MODULE_ERROR_WITH_TYPE(WrongDatatypeOnPortException, ostr.str());
    }
    return data;
  }

}}


namespace Modules
{

  struct SCISHARE MatrixPortTag {};
  struct SCISHARE ScalarPortTag {};
  struct SCISHARE StringPortTag {};
  struct SCISHARE FieldPortTag {};
  struct SCISHARE MeshPortTag {}; //TODO temporary
  struct SCISHARE GeometryPortTag {};
  struct SCISHARE DatatypePortTag {};

  template <typename Base>
  struct DynamicPortTag : Base 
  {
    typedef Base type;
  };
  
  template <size_t N>
  struct NumInputPorts
  {
    enum { NumIPorts = N };
  };
  
  template <size_t N>
  struct NumOutputPorts
  {
    enum { NumOPorts = N };
  };
  
  struct HasNoInputPorts : NumInputPorts<0> {};
  struct HasNoOutputPorts : NumOutputPorts<0> {};
  
  //MEGA TODO: these will become variadic templates in VS2013
  template <class PortTypeTag>
  class Has1InputPort : public NumInputPorts<1>
  {
  public:
    static std::vector<SCIRun::Dataflow::Networks::InputPortDescription> inputPortDescription(const std::string& port0Name);
  };

  template <class PortTypeTag0, class PortTypeTag1>
  class Has2InputPorts : public NumInputPorts<2>
  {
  public:
    static std::vector<SCIRun::Dataflow::Networks::InputPortDescription> inputPortDescription(const std::string& port0Name, const std::string& port1Name)
    {
      //TODO: use move semantics
      auto ports = Has1InputPort<PortTypeTag0>::inputPortDescription(port0Name);
      ports.push_back(Has1InputPort<PortTypeTag1>::inputPortDescription(port1Name)[0]);
      return ports;
    }
  };

  template <class PortTypeTag0, class PortTypeTag1, class PortTypeTag2>
  class Has3InputPorts : public NumInputPorts<3>
  {
  public:
    static std::vector<SCIRun::Dataflow::Networks::InputPortDescription> inputPortDescription(const std::string& port0Name, const std::string& port1Name, const std::string& port2Name)
    {
      auto ports = Has2InputPorts<PortTypeTag0, PortTypeTag1>::inputPortDescription(port0Name, port1Name);
      ports.push_back(Has1InputPort<PortTypeTag2>::inputPortDescription(port2Name)[0]);
      return ports;
    }
  };

  template <class PortTypeTag0, class PortTypeTag1, class PortTypeTag2, class PortTypeTag3>
  class Has4InputPorts : public NumInputPorts<4>
  {
  public:
    static std::vector<SCIRun::Dataflow::Networks::InputPortDescription> inputPortDescription(const std::string& port0Name, const std::string& port1Name, const std::string& port2Name, const std::string& port3Name)
    {
      auto ports = Has3InputPorts<PortTypeTag0, PortTypeTag1, PortTypeTag2>::inputPortDescription(port0Name, port1Name, port2Name);
      ports.push_back(Has1InputPort<PortTypeTag3>::inputPortDescription(port3Name)[0]);
      return ports;
    }
  };
  
  template <class PortTypeTag0, class PortTypeTag1, class PortTypeTag2, class PortTypeTag3, class PortTypeTag4>
  class Has5InputPorts : public NumInputPorts<5>
  {
  public:
    static std::vector<SCIRun::Dataflow::Networks::InputPortDescription> inputPortDescription(const std::string& port0Name, const std::string& port1Name, const std::string& port2Name, const std::string& port3Name, const std::string& port4Name)
    {
      auto ports = Has4InputPorts<PortTypeTag0, PortTypeTag1, PortTypeTag2, PortTypeTag3>::inputPortDescription(port0Name, port1Name, port2Name, port3Name);
      ports.push_back(Has1InputPort<PortTypeTag4>::inputPortDescription(port4Name)[0]);
      return ports;
    }
  };
  
  template <class PortTypeTag0, class PortTypeTag1, class PortTypeTag2, class PortTypeTag3, class PortTypeTag4, class PortTypeTag5>
  class Has6InputPorts : public NumInputPorts<6>
  {
  public:
    static std::vector<SCIRun::Dataflow::Networks::InputPortDescription> inputPortDescription(const std::string& port0Name, const std::string& port1Name, const std::string& port2Name, const std::string& port3Name, const std::string& port4Name, const std::string& port5Name)
    {
      auto ports = Has5InputPorts<PortTypeTag0, PortTypeTag1, PortTypeTag2, PortTypeTag3, PortTypeTag4>::inputPortDescription(port0Name, port1Name, port2Name, port3Name, port4Name);
      ports.push_back(Has1InputPort<PortTypeTag5>::inputPortDescription(port5Name)[0]);
      return ports;
    }
  };

  template <class PortTypeTag>
  class Has1OutputPort : public NumOutputPorts<1>
  {
  public:
    static std::vector<SCIRun::Dataflow::Networks::OutputPortDescription> outputPortDescription(const std::string& port0Name);
  };

  template <class PortTypeTag0, class PortTypeTag1>
  class Has2OutputPorts : public NumOutputPorts<2>
  {
  public:
    static std::vector<SCIRun::Dataflow::Networks::OutputPortDescription> outputPortDescription(const std::string& port0Name, const std::string& port1Name)
    {
      //TODO: use move semantics
      auto ports = Has1OutputPort<PortTypeTag0>::outputPortDescription(port0Name);
      ports.push_back(Has1OutputPort<PortTypeTag1>::outputPortDescription(port1Name)[0]);
      return ports;
    }
  };

  template <class PortTypeTag0, class PortTypeTag1, class PortTypeTag2>
  class Has3OutputPorts : public NumOutputPorts<3>
  {
  public:
    static std::vector<SCIRun::Dataflow::Networks::OutputPortDescription> outputPortDescription(const std::string& port0Name, const std::string& port1Name, const std::string& port2Name)
    {
      auto ports = Has2OutputPorts<PortTypeTag0, PortTypeTag1>::outputPortDescription(port0Name, port1Name);
      ports.push_back(Has1OutputPort<PortTypeTag2>::outputPortDescription(port2Name)[0]);
      return ports;
    }
  };

  template <class PortTypeTag0, class PortTypeTag1, class PortTypeTag2, class PortTypeTag3>
  class Has4OutputPorts : public NumOutputPorts<4>
  {
  public:
    static std::vector<SCIRun::Dataflow::Networks::OutputPortDescription> outputPortDescription(const std::string& port0Name, const std::string& port1Name, const std::string& port2Name, const std::string& port3Name)
    {
      auto ports = Has3OutputPorts<PortTypeTag0, PortTypeTag1, PortTypeTag2>::outputPortDescription(port0Name, port1Name, port2Name);
      ports.push_back(Has1OutputPort<PortTypeTag3>::outputPortDescription(port3Name)[0]);
      return ports;
    }
  };

  template <class PortTypeTag0, class PortTypeTag1, class PortTypeTag2, class PortTypeTag3, class PortTypeTag4>
  class Has5OutputPorts : public NumOutputPorts<5>
  {
  public:
    static std::vector<SCIRun::Dataflow::Networks::OutputPortDescription> outputPortDescription(const std::string& port0Name, const std::string& port1Name, const std::string& port2Name, const std::string& port3Name, const std::string& port4Name)
    {
      auto ports = Has4OutputPorts<PortTypeTag0, PortTypeTag1, PortTypeTag2, PortTypeTag3>::outputPortDescription(port0Name, port1Name, port2Name, port3Name);
      ports.push_back(Has1OutputPort<PortTypeTag4>::outputPortDescription(port4Name)[0]);
      return ports;
    }
  };

  template <class PortTypeTag0, class PortTypeTag1, class PortTypeTag2, class PortTypeTag3, class PortTypeTag4, class PortTypeTag5>
  class Has6OutputPorts : public NumOutputPorts<6>
  {
  public:
    static std::vector<SCIRun::Dataflow::Networks::OutputPortDescription> outputPortDescription(const std::string& port0Name, const std::string& port1Name, const std::string& port2Name, const std::string& port3Name, const std::string& port4Name, const std::string& port5Name)
    {
      auto ports = Has5OutputPorts<PortTypeTag0, PortTypeTag1, PortTypeTag2, PortTypeTag3, PortTypeTag4>::outputPortDescription(port0Name, port1Name, port2Name, port3Name, port4Name);
      ports.push_back(Has1OutputPort<PortTypeTag5>::outputPortDescription(port5Name)[0]);
      return ports;
    }
  };

#define PORT_SPEC(type)   template <>\
  class Has1InputPort<type ##PortTag> : public NumInputPorts<1>\
  {\
  public:\
    static std::vector<SCIRun::Dataflow::Networks::InputPortDescription> inputPortDescription(const std::string& port0Name)\
    {\
      std::vector<SCIRun::Dataflow::Networks::InputPortDescription> ports;\
      ports.push_back(SCIRun::Dataflow::Networks::PortDescription(SCIRun::Dataflow::Networks::PortId(0, port0Name), #type, false)); \
      return ports;\
    }\
  };\
  template <>\
  class Has1OutputPort<type ##PortTag> : public NumOutputPorts<1>\
  {\
  public:\
    static std::vector<SCIRun::Dataflow::Networks::OutputPortDescription> outputPortDescription(const std::string& port0Name)\
    {\
      std::vector<SCIRun::Dataflow::Networks::OutputPortDescription> ports;\
      ports.push_back(SCIRun::Dataflow::Networks::PortDescription(SCIRun::Dataflow::Networks::PortId(0, port0Name), #type, false)); \
      return ports;\
    }\
  };\
  template <>\
  class Has1InputPort<DynamicPortTag<type ## PortTag>> : public NumInputPorts<1>\
  {\
  public:\
    static std::vector<SCIRun::Dataflow::Networks::InputPortDescription> inputPortDescription(const std::string& port0Name)\
    {\
      std::vector<SCIRun::Dataflow::Networks::InputPortDescription> ports;\
      ports.push_back(SCIRun::Dataflow::Networks::PortDescription(SCIRun::Dataflow::Networks::PortId(0, port0Name), #type, true)); \
      return ports;\
    }\
  }\

  PORT_SPEC(Matrix);
  PORT_SPEC(Scalar);
  PORT_SPEC(String);
  PORT_SPEC(Field);
  PORT_SPEC(Mesh);  //TODO temporary
  PORT_SPEC(Geometry);
  //PORT_SPEC(DynamicPortTag<Geometry>::type);
  PORT_SPEC(Datatype);

#define ATTACH_NAMESPACE(type) Core::Datatypes::type
#define ATTACH_NAMESPACE2(type) SCIRun::Core::Datatypes::type

#define INPUT_PORT(index, name, type) static std::string inputPort ## index ## Name() { return #name; } \
  StaticPortName< ATTACH_NAMESPACE(type), index > name;

#define INPUT_PORT_DYNAMIC(index, name, type) static std::string inputPort ## index ## Name() { return #name; } \
  DynamicPortName< ATTACH_NAMESPACE(type), index > name;

#define OUTPUT_PORT(index, name, type) static std::string outputPort ## index ## Name() { return #name; } \
  StaticPortName< ATTACH_NAMESPACE(type), index> name;

#define INITIALIZE_PORT(nameObj) do{ nameObj.id_.name = #nameObj; }while(0);

  //TODO: make metafunc for Input/Output

  template <size_t numPorts, class ModuleType>
  struct IPortDescriber
  {
    static std::vector<SCIRun::Dataflow::Networks::InputPortDescription> inputs()
    {
      return std::vector<SCIRun::Dataflow::Networks::InputPortDescription>();
    }
  };

  template <class ModuleType>
  struct IPortDescriber<1, ModuleType>
  {
    static std::vector<SCIRun::Dataflow::Networks::InputPortDescription> inputs()
    {
      return ModuleType::inputPortDescription(ModuleType::inputPort0Name());
    }
  };

  template <class ModuleType>
  struct IPortDescriber<2, ModuleType>
  {
    static std::vector<SCIRun::Dataflow::Networks::InputPortDescription> inputs()
    {
      return ModuleType::inputPortDescription(ModuleType::inputPort0Name(), ModuleType::inputPort1Name());
    }
  };

  template <class ModuleType>
  struct IPortDescriber<3, ModuleType>
  {
    static std::vector<SCIRun::Dataflow::Networks::InputPortDescription> inputs()
    {
      return ModuleType::inputPortDescription(ModuleType::inputPort0Name(), ModuleType::inputPort1Name(), ModuleType::inputPort2Name());
    }
  };

  template <class ModuleType>
  struct IPortDescriber<4, ModuleType>
  {
    static std::vector<SCIRun::Dataflow::Networks::InputPortDescription> inputs()
    {
      return ModuleType::inputPortDescription(ModuleType::inputPort0Name(), ModuleType::inputPort1Name(), ModuleType::inputPort2Name(), ModuleType::inputPort3Name());
    }
  };
  
  template <class ModuleType>
  struct IPortDescriber<5, ModuleType>
  {
    static std::vector<SCIRun::Dataflow::Networks::InputPortDescription> inputs()
    {
      return ModuleType::inputPortDescription(ModuleType::inputPort0Name(), ModuleType::inputPort1Name(), ModuleType::inputPort2Name(), ModuleType::inputPort3Name(), ModuleType::inputPort4Name());
    }
  };
  
  template <class ModuleType>
  struct IPortDescriber<6, ModuleType>
  {
    static std::vector<SCIRun::Dataflow::Networks::InputPortDescription> inputs()
    {
      return ModuleType::inputPortDescription(ModuleType::inputPort0Name(), ModuleType::inputPort1Name(), ModuleType::inputPort2Name(), ModuleType::inputPort3Name(), ModuleType::inputPort4Name(), ModuleType::inputPort5Name());
    }
  };

  template <size_t numPorts, class ModuleType>
  struct OPortDescriber
  {
    static std::vector<SCIRun::Dataflow::Networks::OutputPortDescription> outputs()
    {
      return std::vector<SCIRun::Dataflow::Networks::OutputPortDescription>();
    }
  };

  template <class ModuleType>
  struct OPortDescriber<1, ModuleType>
  {
    static std::vector<SCIRun::Dataflow::Networks::OutputPortDescription> outputs()
    {
      return ModuleType::outputPortDescription(ModuleType::outputPort0Name());
    }
  };

  template <class ModuleType>
  struct OPortDescriber<2, ModuleType>
  {
    static std::vector<SCIRun::Dataflow::Networks::OutputPortDescription> outputs()
    {
      return ModuleType::outputPortDescription(ModuleType::outputPort0Name(), ModuleType::outputPort1Name());
    }
  };

  template <class ModuleType>
  struct OPortDescriber<3, ModuleType>
  {
    static std::vector<SCIRun::Dataflow::Networks::OutputPortDescription> outputs()
    {
      return ModuleType::outputPortDescription(ModuleType::outputPort0Name(), ModuleType::outputPort1Name(), ModuleType::outputPort2Name());
    }
  };

  template <class ModuleType>
  struct OPortDescriber<4, ModuleType>
  {
    static std::vector<SCIRun::Dataflow::Networks::OutputPortDescription> outputs()
    {
      return ModuleType::outputPortDescription(ModuleType::outputPort0Name(), ModuleType::outputPort1Name(), ModuleType::outputPort2Name(), ModuleType::outputPort3Name());
    }
  };

  template <class ModuleType>
  struct OPortDescriber<5, ModuleType>
  {
    static std::vector<SCIRun::Dataflow::Networks::OutputPortDescription> outputs()
    {
      return ModuleType::outputPortDescription(ModuleType::outputPort0Name(), ModuleType::outputPort1Name(), ModuleType::outputPort2Name(), ModuleType::outputPort3Name(), ModuleType::outputPort4Name());
    }
  };

  template <class ModuleType>
  struct OPortDescriber<6, ModuleType>
  {
    static std::vector<SCIRun::Dataflow::Networks::OutputPortDescription> outputs()
    {
      return ModuleType::outputPortDescription(ModuleType::outputPort0Name(), ModuleType::outputPort1Name(), ModuleType::outputPort2Name(), ModuleType::outputPort3Name(), ModuleType::outputPort4Name(), ModuleType::outputPort5Name());
    }
  };

}
}

#endif
