/**************************************************************************************
Copyright 2015 Applied Research Associates, Inc.
Licensed under the Apache License, Version 2.0 (the "License"); you may not use
this file except in compliance with the License. You may obtain a copy of the License
at:
http://www.apache.org/licenses/LICENSE-2.0
Unless required by applicable law or agreed to in writing, software distributed under
the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
CONDITIONS OF ANY KIND, either express or implied. See the License for the
specific language governing permissions and limitations under the License.
**************************************************************************************/

#include "stdafx.h"
#include "Endocrine.h"
#include "system/physiology/SEDrugSystem.h"
#include "system/physiology/SECardiovascularSystem.h"
#include "system/physiology/SEEnergySystem.h"

#include "patient/SEMeal.h"
#include "patient/SENutrition.h"
#include "patient/conditions/SEConsumeMeal.h"

#include "properties/SEScalarPower.h"
#include "properties/SEScalarPressure.h"
#include "properties/SEScalarMass.h"
#include "properties/SEScalarMassPerVolume.h"
#include "properties/SEScalarAmountPerTime.h"
#include "properties/SEScalarMassPerAmount.h"
#include "properties/SEScalarVolume.h"
#include "properties/SEScalarTemperature.h"
#include "properties/SEScalar0To1.h"


Endocrine::Endocrine(BioGears& bg) : SEEndocrineSystem(bg.GetLogger()), m_data(bg)
{
  Clear();
}

Endocrine::~Endocrine()
{
  Clear();
}

void Endocrine::Clear()
{
  SEEndocrineSystem::Clear();
  m_aortaGlucose = nullptr;
  m_aortaEpinephrine = nullptr;
  m_splanchnicInsulin = nullptr;  
}

//--------------------------------------------------------------------------------------------------
/// \brief
/// Initializes system properties to valid homeostatic values.
//--------------------------------------------------------------------------------------------------
void Endocrine::Initialize()
{
  BioGearsSystem::Initialize();
}

bool Endocrine::Load(const CDM::BioGearsEndocrineSystemData& in)
{
  if (!SEEndocrineSystem::Load(in))
    return false;
  BioGearsSystem::LoadState();
  return true;
}
CDM::BioGearsEndocrineSystemData* Endocrine::Unload() const
{
  CDM::BioGearsEndocrineSystemData* data = new CDM::BioGearsEndocrineSystemData();
  Unload(*data);
  return data;
}
void Endocrine::Unload(CDM::BioGearsEndocrineSystemData& data) const
{
  SEEndocrineSystem::Unload(data);
}

void Endocrine::SetUp()
{
  m_dt_s = m_data.GetTimeStep().GetValue(TimeUnit::s);
  SELiquidCompartment* aorta = m_data.GetCompartments().GetLiquidCompartment(BGE::VascularCompartment::Aorta);
  SELiquidCompartment* rkidney = m_data.GetCompartments().GetLiquidCompartment(BGE::VascularCompartment::RightEfferentArteriole);
  SELiquidCompartment* lkidney = m_data.GetCompartments().GetLiquidCompartment(BGE::VascularCompartment::LeftEfferentArteriole);
  m_aortaEpinephrine = aorta->GetSubstanceQuantity(m_data.GetSubstances().GetEpi());
  m_rKidneyEpinephrine = rkidney->GetSubstanceQuantity(m_data.GetSubstances().GetEpi());
  m_lKidneyEpinephrine = lkidney->GetSubstanceQuantity(m_data.GetSubstances().GetEpi());
  m_aortaGlucose = aorta->GetSubstanceQuantity(m_data.GetSubstances().GetGlucose());
  SESubstance* insulin = &m_data.GetSubstances().GetInsulin();
  m_insulinMolarMass_g_Per_mol = insulin->GetMolarMass(MassPerAmountUnit::g_Per_mol);
  SESubstance* glucagon = &m_data.GetSubstances().GetGlucagon();
  m_glucagonMolarMass_g_Per_mol = glucagon->GetMolarMass(MassPerAmountUnit::g_Per_mol);
  m_splanchnicInsulin = m_data.GetCompartments().GetLiquidCompartment(BGE::VascularCompartment::Splanchnic)->GetSubstanceQuantity(*insulin);
  m_splanchnicGlucagon = m_data.GetCompartments().GetLiquidCompartment(BGE::VascularCompartment::Splanchnic)->GetSubstanceQuantity(*glucagon);
}

void Endocrine::AtSteadyState()
{

}

//--------------------------------------------------------------------------------------------------
/// \brief
/// Endocrine Preprocess function
///
/// \details
/// Currently, only three hormones exist in the BioGears system: epinephrine, insulin, and glucagon. 
/// These functions determine the release of these hormones.
/// The hormones will then circulate using the transport and substances methodologies.
//--------------------------------------------------------------------------------------------------
void Endocrine::PreProcess()
{
  ReleaseEpinephrine();
  SynthesizeInsulin();
  SynthesizeGlucagon();
}

//--------------------------------------------------------------------------------------------------
/// \brief
/// Endocrine process function
///
/// \details
/// The Endocrine system does not currently have any Process functionality.
//--------------------------------------------------------------------------------------------------
void Endocrine::Process()
{
  
}

//--------------------------------------------------------------------------------------------------
/// \brief
/// Endocrine PostProcess function
///
/// \details
/// The Endocrine system does not currently have any PostProcess functionality.
//--------------------------------------------------------------------------------------------------
void Endocrine::PostProcess()
{

}

//--------------------------------------------------------------------------------------------------
/// \brief
/// Calculate the rate of insulin production 
///
/// \details
/// The insulin production rate is calculated based on the relevant range of glucose and instantaneous concentration of glucose in the aorta
/// (representative of the body). The equation for insulin production is from \cite tolic2000insulin
//--------------------------------------------------------------------------------------------------
void Endocrine::SynthesizeInsulin()
{	
	double bloodGlucoseConcentration_g_Per_L = m_aortaGlucose->GetConcentration(MassPerVolumeUnit::g_Per_L);
  // 2.0 = upperConcentration_g_Per_L
	// 0.3 = lowerConcentration_g_Per_l
	// 65.421 = amplitudeRate_mU_Per_min
  // 6.67 = insulinConversionToAmount_pmol_Per_mU

  // Note: Guyton says insulin production at 90 mg/dL glucose concentration should be
  // 25 ng/min/kg, which is about 300 pmol/min, double what we have using this curve from Tolic.
  // Because of this, we won't capture insulin behavior for very high glucose concentrations, see Guyton p 991
  // If we ever want to capture diabetes behavior, we'll need to change this curve.
  // Also, since we only key off of the instantaneous aorta glucose, we miss out on any parasympathetic
  // signals that affect Beta cells, meaning if we implement stress response, we might also see hyperinsulinemia (see Boron p 1222)
	double insulinSynthesisRate_pmol_Per_min = 6.67 * 65.421 / (1.0 + exp((2.0 - 2.0*bloodGlucoseConcentration_g_Per_L) / 0.3));

	GetInsulinSynthesisRate().SetValue(insulinSynthesisRate_pmol_Per_min, AmountPerTimeUnit::pmol_Per_min);
	
	double insulinMassDelta_g = Convert(insulinSynthesisRate_pmol_Per_min, AmountPerTimeUnit::pmol_Per_min, AmountPerTimeUnit::mol_Per_s);
	insulinMassDelta_g *= m_insulinMolarMass_g_Per_mol*m_dt_s;
	
  m_splanchnicInsulin->GetMass().IncrementValue(insulinMassDelta_g, MassUnit::g);
  m_splanchnicInsulin->Balance(BalanceLiquidBy::Mass);
}

//--------------------------------------------------------------------------------------------------
/// \brief
/// Calculate the rate of glucagon production 
///
/// \details
/// The glucagon production rate is set based on the aorta glucose concentration to values that
/// allow it to have stable concentrations when paired with the clearance rate of 9 mL/min kg from 
/// https://www.ncbi.nlm.nih.gov/pubmed/773949
//--------------------------------------------------------------------------------------------------
void Endocrine::SynthesizeGlucagon()
{
  double bloodGlucoseConcentration_g_Per_L = m_aortaGlucose->GetConcentration(MassPerVolumeUnit::g_Per_L);
  double glucagonSynthesisRate_pmol_Per_min = 21.3 - (21.3 / (1.0 + exp((2 - 2 * bloodGlucoseConcentration_g_Per_L) / .3)));

  GetGlucagonSynthesisRate().SetValue(glucagonSynthesisRate_pmol_Per_min, AmountPerTimeUnit::pmol_Per_min);

  double glucagonMassDelta_g = Convert(glucagonSynthesisRate_pmol_Per_min, AmountPerTimeUnit::pmol_Per_min, AmountPerTimeUnit::mol_Per_s);
  glucagonMassDelta_g *= m_glucagonMolarMass_g_Per_mol*m_dt_s;

  m_splanchnicGlucagon->GetMass().IncrementValue(glucagonMassDelta_g, MassUnit::g);
  m_splanchnicGlucagon->Balance(BalanceLiquidBy::Mass);
}

//--------------------------------------------------------------------------------------------------
/// \brief
/// Release epinephrine into the bloodstream and handle sympathetic responses
///
/// \details
/// Epinephrine is released at a basal rate of .18 ug/min \cite best1982release from the kidneys. During
/// certain events, the release rate of epinephrine increases. This is sympathetic response.
//--------------------------------------------------------------------------------------------------
void Endocrine::ReleaseEpinephrine()
{
  SEPatient& Patient = m_data.GetPatient();
  double patientWeight_kg = Patient.GetWeight(MassUnit::kg);
  double epinephrineBasalReleaseRate_ug_Per_min = .00229393 * patientWeight_kg; //We want it to be ~.18 ug/min for our StandardMale
  double epinephrineRelease_ug = (epinephrineBasalReleaseRate_ug_Per_min / 60) * m_dt_s;  //amount released per timestep

  double currentMetabolicRate_W = m_data.GetEnergy().GetTotalMetabolicRate(PowerUnit::W);
  double basalMetabolicRate_W = Patient.GetBasalMetabolicRate(PowerUnit::W);
  double releaseMultiplier = 1.0;

  // If we have exercise, release more epi. Release multiplier is a sigmoid based on the total metabolic rate
  // with the maximum multiplier adapted from concentration data presented in @cite tidgren1991renal and @cite stratton1985hemodynamic
  // and the shape adjusted to match data in @cite tidgren1991renal.
  if (currentMetabolicRate_W > basalMetabolicRate_W)
  {
    double exercise_W = (currentMetabolicRate_W - basalMetabolicRate_W);
    double e50_W = 190;
    double eta = 0.035;
    double maxMultiplier = 18.75; 
    releaseMultiplier = 1.0 + GeneralMath::LogisticFunction(maxMultiplier, e50_W, eta, exercise_W);
  }

  // If we have a stress/anxiety response, release more epi
  if (m_data.GetActions().GetPatientActions().HasAcuteStress())
  {
    SEAcuteStress* s = m_data.GetActions().GetPatientActions().GetAcuteStress();
    double severity = s->GetSeverity().GetValue();

    //The highest stress multiplier we currently support is 30
    releaseMultiplier += GeneralMath::LinearInterpolator(0, 1, 0, 30, severity);
  }

  epinephrineRelease_ug *= releaseMultiplier;

  m_rKidneyEpinephrine->GetMass().IncrementValue(0.5*epinephrineRelease_ug, MassUnit::ug);
  m_lKidneyEpinephrine->GetMass().IncrementValue(0.5*epinephrineRelease_ug, MassUnit::ug);
}