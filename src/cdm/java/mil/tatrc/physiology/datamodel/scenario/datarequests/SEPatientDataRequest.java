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

package mil.tatrc.physiology.datamodel.scenario.datarequests;

import mil.tatrc.physiology.datamodel.CDMSerializer;
import mil.tatrc.physiology.datamodel.bind.PatientDataRequestData;

public class SEPatientDataRequest extends SEDataRequest
{
	public SEPatientDataRequest()
	{
		
	}
	
	public void reset()
	{
		super.reset();
	}
	
	public boolean load(PatientDataRequestData in)
	{
		super.load(in);
		return true;
	}
	
	public PatientDataRequestData unload()
	{
		PatientDataRequestData data = CDMSerializer.objFactory.createPatientDataRequestData();
		unload(data);
		return data;
	}
	
	protected void unload(PatientDataRequestData data)
	{
		super.unload(data);
	}
}