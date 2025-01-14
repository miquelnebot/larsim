////////////////////////////////////////////////////////////////////////
// Class:       EventWeight
// Module Type: producer
// File:        EventWeight_module.cc
//
// Generated at Fri Mar 20 09:36:11 2015 by Zarko Pavlovic using artmod
// from cetpkgsupport v1_08_04.
//
// Ported from uboonecode to larsim on Feb 14 2018 by Marco Del Tutto
////////////////////////////////////////////////////////////////////////

#include "art/Framework/Core/EDProducer.h"
#include "art/Framework/Core/ModuleMacros.h"
#include "art/Framework/Principal/Event.h"
#include "art/Framework/Principal/Handle.h"
#include "nugen/EventGeneratorBase/GENIE/GENIE2ART.h"
#include "fhiclcpp/ParameterSet.h"
#include "messagefacility/MessageLogger/MessageLogger.h"

#include <memory>
#include <iostream>
#include <iomanip>

#include "larsim/EventWeight/Base/Weight_t.h"
#include "larsim/EventWeight/Base/MCEventWeight.h"
#include "larsim/EventWeight/Base/WeightManager.h"

#include "nusimdata/SimulationBase/MCTruth.h"
#include "nugen/EventGeneratorBase/GENIE/GENIE2ART.h"

namespace evwgh {

  class EventWeight : public art::EDProducer {

  public:

    explicit EventWeight(fhicl::ParameterSet const & p);
    // The destructor generated by the compiler is fine for classes
    // without bare pointers or other resource use.

    // Plugins should not be copied or assigned.
    EventWeight(EventWeight const &) = delete;
    EventWeight(EventWeight &&) = delete;
    EventWeight & operator = (EventWeight const &) = delete;
    EventWeight & operator = (EventWeight &&) = delete;

  private:

    // Required functions.
    void produce(art::Event & e) override;

    //Optional functions.
    void endJob() override;

    WeightManager _wgt_manager;
    std::string fGenieModuleLabel;
  };

  EventWeight::EventWeight(fhicl::ParameterSet const & p)
    : EDProducer{p}
    , fGenieModuleLabel{p.get<std::string>("genie_module_label", "generator")}
  {
    // Configure the appropriate GENIE tune if needed (important for v3+ only)
    // NOTE: In all normal use cases, relying on the ${GENIE_XSEC_TUNE}
    // environment variable set by the genie_xsec package should be sufficient.
    // Only include the "TuneName" FHiCL parameter for EventWeight if you
    // really know what you're doing! The same goes for the
    // "EventGeneratorList" parameter.
    std::string genie_tune_name = p.get<std::string>("TuneName",
                                                     "${GENIE_XSEC_TUNE}");

    // The default empty string used here will cause the subsequent call to
    // evgb::SetEventGeneratorListAndTune() to leave GENIE's current event
    // generator list name (probably "Default") alone
    std::string evgen_list_name = p.get<std::string>("EventGeneratorList", "");

    // Tell GENIE about the event generator list and tune
    evgb::SetEventGeneratorListAndTune( evgen_list_name, genie_tune_name );

    auto const n_func = _wgt_manager.Configure(p, *this);
    if ( n_func > 0 )
      produces<std::vector<MCEventWeight> >();
  }

  void EventWeight::produce(art::Event & e)
  {
    // Implementation of required member function here.
    auto mcwghvec = std::make_unique<std::vector<MCEventWeight>>();

    // Get the MC generator information out of the event
    // these are all handles to mc information.
    std::vector<art::Ptr<simb::MCTruth> > mclist;

      // Actually go and get the stuff
    auto const mcTruthHandle = e.getValidHandle<std::vector<simb::MCTruth>>(fGenieModuleLabel);
      art::fill_ptr_vector(mclist, mcTruthHandle);

    // Loop over all neutrinos in this event
    for (unsigned int inu = 0; inu < mclist.size(); ++inu) {
      auto const mcwgh = _wgt_manager.Run(e, inu);
      mcwghvec->push_back(mcwgh);
    }

    e.put(std::move(mcwghvec));
  }

  void EventWeight::endJob()
  {
    // Get the map from sting to Weight_t from the manager
    std::map<std::string, Weight_t*> weightCalcMap = _wgt_manager.GetWeightCalcMap();

    std::stringstream job_summary;
    job_summary  <<  std::setprecision(2);
    for (int i=1; i <= 110 ;i++) job_summary << "=";
    job_summary << std::endl;
    job_summary << std::setw(20) << "WeightCalc"
                << std::setw(15) << "Type"
                << std::setw(15) << "#RW neutrinos"
                << std::setw(15) << "#Multisims"
                << std::setw(15) << "Min"
                << std::setw(15) << "Max"
                << std::setw(15) << "Avg"
                << std::endl;
    for (int i=1; i <= 110; i++) job_summary << "=";
    job_summary << std::endl;
    for (auto it = weightCalcMap.begin(); it!=weightCalcMap.end(); it++) {
      job_summary << std::setw(20) << it->first
                  << std::setw(15) << (it->second->fWeightCalcType)
                  << std::setw(15) << (it->second->fNcalls)
                  << std::setw(15) << (it->second->fNmultisims)
                  << std::setw(15) << (it->second->fMinWeight)
                  << std::setw(15) << (it->second->fMaxWeight)
                  << std::setw(15) << (it->second->fAvgWeight)
                  << std::endl;
    }
    for (int i=1; i<=110; i++) job_summary << "=";
    job_summary << std::endl;
    mf::LogInfo("") << job_summary.str();
  }

} // namespace

DEFINE_ART_MODULE(evwgh::EventWeight)
