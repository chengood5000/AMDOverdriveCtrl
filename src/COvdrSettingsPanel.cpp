/*******************************************************************************

 * This program reads HW information from your ATI Radeon card and displays them
 * You can also change frequencies and voltages.

 * THIS PROGRAM MAY DAMAGE YOUR VIDEO CARD, IF YOU APPLY NONSENSIAL VALUES.
 * e.g. INCREASING THE VOLTAGES AND FREQUENCIES IN CONJUNCTION WITH LOWERING THE
 *      FAN SPEED IS NOT ADVISABLE!

 * Copyright(C) Thorsten Gilling (tgilling@web.de)

 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*******************************************************************************/


#include <wx/event.h>
#include "COvdrSettingsPanel.h"
#include "adl.h"
#include "main.h"

COvdrSettingsPanel::COvdrSettingsPanel(wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style)
    : COvdrSettingsPanelBase(parent, id, pos, size, style)
    , mpSettingsPanelLow(NULL)
    , mpSettingsPanelMid(NULL)
    , mpSettingsPanelHigh(NULL)
    , mGPU(NULL)
    , mMem(NULL)
    , mVoltage(NULL)
    
{
    adl = ADL::Instance();

    if(!(adl->GetSupportedFeatures() & ADL::FEAT_GET_OD_PARAMETERS) ||
       !(adl->GetSupportedFeatures() & ADL::FEAT_GET_OD_PERF_LEVELS))
    {
        Show(false);
    }
    else
    {	
	mGPU = new int[adl->mODParameters.iNumberOfPerformanceLevels];
	mMem = new int[adl->mODParameters.iNumberOfPerformanceLevels];
	mVoltage = new int[adl->mODParameters.iNumberOfPerformanceLevels];
	
	if (mGPU == NULL || mMem == NULL || mVoltage == NULL)
	{
	    wxMessageBox(wxT("Unable to create Overdrive Panels (out of memory)"), wxT("ERROR"), wxOK|wxCENTRE|wxICON_ERROR);
	}
    
	if (adl->mODParameters.iNumberOfPerformanceLevels == 3)
	{
	    mpSettingsPanelLow = new CSettingsPanel(0, mOvdrNotebook);
	    mpSettingsPanelMid = new CSettingsPanel(1, mOvdrNotebook);
	    mpSettingsPanelHigh = new CSettingsPanel(2, mOvdrNotebook);	    
	}
	else 
	{
	    mpSettingsPanelLow = new CSettingsPanel(0, mOvdrNotebook);
	    mpSettingsPanelHigh = new CSettingsPanel(1, mOvdrNotebook);	    	    
	}

        if(mpSettingsPanelLow != NULL 
	    && (mpSettingsPanelMid != NULL || adl->mODParameters.iNumberOfPerformanceLevels == 2)
	    && mpSettingsPanelHigh != NULL)
        {
            mOvdrNotebook->AddPage(mpSettingsPanelLow, wxT("Low"));
	    if (adl->mODParameters.iNumberOfPerformanceLevels == 3)
	    {           
		mOvdrNotebook->AddPage(mpSettingsPanelMid, wxT("Mid"));
	    }
            mOvdrNotebook->AddPage(mpSettingsPanelHigh, wxT("High"));
        }
        else
        {
            wxMessageBox(wxT("Unable to create Overdrive Panels"), wxT("ERROR"), wxOK|wxCENTRE|wxICON_ERROR);
        }
    }
}

COvdrSettingsPanel::~COvdrSettingsPanel()
{
    if (mGPU != NULL)
    {
	delete[] mGPU;
	mGPU = NULL;
    }
    if (mGPU != NULL)
    {
	delete[] mMem;
	mMem = NULL;
    }
    if (mGPU != NULL)
    {
	delete[] mVoltage;
	mVoltage = NULL;
    }
}	

void COvdrSettingsPanel::SetOverdriveValues(int PerfLevel, int gpu, int mem, int volt)
{
    if((adl->GetSupportedFeatures() & ADL::FEAT_GET_OD_PARAMETERS) && (adl->GetSupportedFeatures() & ADL::FEAT_GET_OD_PERF_LEVELS))
    {
	if(PerfLevel >= 0 && PerfLevel < adl->mODParameters.iNumberOfPerformanceLevels)
	{
	    mGPU[PerfLevel] = gpu;
	    mMem[PerfLevel] = mem;
	    mVoltage[PerfLevel] = volt;
	}
    }
}

bool COvdrSettingsPanel::CommitOverdriveValues()
{
    if((adl->GetSupportedFeatures() & ADL::FEAT_GET_OD_PARAMETERS) && (adl->GetSupportedFeatures() & ADL::FEAT_GET_OD_PERF_LEVELS))
    {
#ifdef FAKE_ATI_CARD
        for(int i=0; i<adl->mODParameters.iNumberOfPerformanceLevels; i++)
        {
            ACT_LOG("SetOverdriveValues: Level " << i << " " << mGPU[i] << "MHz, " << mMem[i] << "MHz, " <<	"MHz, " << (double)mVoltage[i]/1000.0 << "V");
        }
        return true;
#endif

        ADLODParameters para;
        ADLODPerformanceLevels* levels;

        ADL* adl = ADL::Instance();

        if(SAVE_CALL(adl->ADL_Overdrive5_ODParameters_Get)(adl->GetGPUIndex(), &para) == ADL_OK)
        {
            int perf_level_size = sizeof(ADLODPerformanceLevels) + sizeof(ADLODPerformanceLevel) * (para.iNumberOfPerformanceLevels - 1);
            levels = (ADLODPerformanceLevels*)malloc(perf_level_size);
            levels->iSize = perf_level_size;

            if(SAVE_CALL(adl->ADL_Overdrive5_ODPerformanceLevels_Get)(adl->GetGPUIndex(), 0, levels) == ADL_OK)
            {
                for(int i=0; i<para.iNumberOfPerformanceLevels; i++)
                {
		    if (mGPU[i] != 0)
		    { 
			levels->aLevels[i].iEngineClock = mGPU[i] * 100;
		    }
		    
		    if (mMem[i] != 0)
		    {
			levels->aLevels[i].iMemoryClock = mMem[i] * 100;
		    }
		    
		    if (mVoltage[i] != 0)
		    {
			levels->aLevels[i].iVddc = mVoltage[i];			
		    }

                    ACT_LOG("SetOverdriveValues: Level " << i << ": " << levels->aLevels[i].iEngineClock << "MHz, " 
			<< levels->aLevels[i].iMemoryClock << "MHz, " << "MHz, " 
			<< (double)levels->aLevels[i].iVddc/1000.0 << "V");
                }
            }

            if(SAVE_CALL(adl->ADL_Overdrive5_ODPerformanceLevels_Set)(adl->GetGPUIndex(), levels) != ADL_OK)
            {
                return false;
            }

            free(levels);
            levels = NULL;

            return true;
        }
        return false;
    }
    return true;
}

void COvdrSettingsPanel::UpdateDisplayValues()
{
    if(mpSettingsPanelLow != NULL && mpSettingsPanelMid != NULL && mpSettingsPanelHigh != NULL)
    {
        mpSettingsPanelLow->UpdateDisplayValues();
	if (adl->mODParameters.iNumberOfPerformanceLevels == 3)
	{       
	    mpSettingsPanelMid->UpdateDisplayValues();
	}
        mpSettingsPanelHigh->UpdateDisplayValues();
    }
}

void COvdrSettingsPanel::mButtonResetClick(wxCommandEvent& WXUNUSED(event))
{
    int level0, level1, level2;
    int gpu0, gpu1, gpu2;
    int mem0, mem1, mem2;
    int v0, v1, v2;

    mpSettingsPanelLow->GetResetValues(level0, gpu0, mem0, v0);    
    SetOverdriveValues(level0, gpu0, mem0, v0);
    
    if (adl->mODParameters.iNumberOfPerformanceLevels == 3)
    {
	mpSettingsPanelMid->GetResetValues(level1, gpu1, mem1, v1);
	SetOverdriveValues(level1, gpu1, mem1, v1);
	mpSettingsPanelHigh->GetResetValues(level2, gpu2, mem2, v2);
	SetOverdriveValues(level2, gpu2, mem2, v2);    
    }	
    else
    {
	mpSettingsPanelHigh->GetResetValues(level1, gpu1, mem1, v1);
	SetOverdriveValues(level1, gpu1, mem1, v1);    	
    }
    
    CommitOverdriveValues();

    UpdateDisplayValues();
}

void COvdrSettingsPanel::mButtonSetClick(wxCommandEvent& WXUNUSED(event))
{
    int level0, level1, level2;
    int gpu0, gpu1, gpu2;
    int mem0, mem1, mem2;
    int v0, v1, v2;

    mpSettingsPanelLow->GetValues(level0, gpu0, mem0, v0);
    
    if (adl->mODParameters.iNumberOfPerformanceLevels == 3)
    {    
	mpSettingsPanelMid->GetValues(level1, gpu1, mem1, v1);
	mpSettingsPanelHigh->GetValues(level2, gpu2, mem2, v2);
    }
    else
    {
	mpSettingsPanelHigh->GetValues(level1, gpu1, mem1, v1);
	mem2 = mem1;
	gpu2 = gpu1;
	v2 = v1;
    }

    wxString tmp = wxString::FromAscii(adl->mpAdapterInfo->strAdapterName);
    if(tmp.Find(wxT("Mobility")) == wxNOT_FOUND)
    {
        // mobility chips seem to be OK, maybe only the HD48xx series has a problem, I don't know... so warn if desktop HW is detected
        if(mem0 != mem1 || mem1 != mem2)
        {
            if(wxMessageBox(wxT("You have choosen different memory clock settings\nfor the three performance levels.\n\n")
                            wxT("This may result in screen flickering on certain hardware.\n\n")
                            wxT("Are you sure you want to set these values?"), wxT("Are you sure?"), wxYES_NO|wxCENTRE|wxICON_WARNING) == wxNO)
            {
                return;
            }
        }
    }

    if(gpu0 <= gpu1 && gpu1 <= gpu2 && mem0 <= mem1 && mem1 <= mem2)
    {
        SetOverdriveValues(level0, gpu0, mem0, v0);
        SetOverdriveValues(level1, gpu1, mem1, v1);
	if (adl->mODParameters.iNumberOfPerformanceLevels == 3)
	{    
	    SetOverdriveValues(level2, gpu2, mem2, v2);
	}

        CommitOverdriveValues();
    }
    else
    {
        wxMessageBox(wxT("\nThe choosen overdrive settings are not valid.\n\nFrequency settings must follow the rule:\n\n"
                         "low level <= mid level <= high level"), wxT("Invalid settings"), wxOK|wxCENTRE|wxICON_ERROR);
    }
}
void COvdrSettingsPanel::mButtonSaveDefaultClick(wxCommandEvent& WXUNUSED(event))
{
    if(wxMessageBox(wxT("Saving these overdrive settings will override any previously saved default settings in ~/.AMDOverDriveCtrl/default.ovdr.\n\n")
                    wxT("Override default settings?"), wxT("Override default settings?"), wxYES_NO|wxICON_QUESTION) == wxYES)
    {
        // Call mButtonSetClick to update Overdrive values
        wxCommandEvent event;
        this->mButtonSetClick(event);

        if(getenv("HOME"))
        {

            // Get path of default.ovdr
            wxString file_path = wxString::FromAscii(getenv("HOME"));
            file_path += wxT("/.AMDOverdriveCtrl/default.ovdr");

            MainDialog* mMD;
            mMD = (MainDialog*)this->GetParent()->GetParent(); // get a pointer to the MainDialog object
            mMD->SaveXML(file_path); // reuse MainDialog::SaveXML

            INF_LOG("mButtonSaveDefaultClick: saved " << file_path.mb_str(wxConvUTF8));
        }
    }
}
