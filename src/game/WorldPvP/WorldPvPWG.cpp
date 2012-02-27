/*
 * Copyright (C) 2005-2012 MaNGOS <http://getmangos.com/>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#include "WorldPvP.h"
#include "WorldPvPWG.h"
#include "../Battleground.h"
#include "../Player.h"
#include "../GameObject.h"
//#include "../Transports.h"
#include "../Chat.h"



WorldPvPWG::WorldPvPWG() : WorldPvP(),
    m_uiZoneController(ALLIANCE)   //need suport
{
    m_uiTypeId = WORLD_PVP_TYPE_WG;

    // init world states

    InitBattlefield();
}

bool WorldPvPWG::InitWorldPvPArea()
{
    RegisterZone(ZONE_ID_WINTERGRASP);

    return true;
}

void WorldPvPWG::InitBattlefield()
{
    m_bIsBattleStarted = true;

    // set initial timer (check time left to next battle)
    // TODO: store timer in world database(?)
    // m_uiNextBattleStartTimer = (2*HOUR + 30*MINUTE) * IN_MILLISECONDS;
    m_uiNextBattleStartTimer = 60000;
}

void WorldPvPWG::StartBattle()
{
    //Need add player list. 
    for (WintergraspPlayerMap::const_iterator itr = GetPlayers().begin(); itr != GetPlayers().end(); ++itr)
    {
        Player *pPlayer = sObjectMgr.GetPlayer(itr->first);

        if(pPlayer->GetTeam() != GetDefender())
        {
            HandlePlayerRemoveAuras();
            pPlayer->RemoveAurasDueToSpell(SPELL_AURA_FLY);
            pPlayer->RemoveAurasDueToSpell(SPELL_TOWER_CONTROL);
            pPlayer->RemoveAurasDueToSpell(SPELL_SPIRITUAL_IMMUNITY);
            if(pPlayer->getLevel() < 75)
                pPlayer->CastSpell(pPlayer, SPELL_TELEPORT_DALARAN, true);
            else
            {
                //CountAtk++;
                pPlayer->RemoveAurasDueToSpell(SPELL_AURA_FLY);
                pPlayer->CastSpell(pPlayer, SPELL_PARAHUTE, true);              //prevent die if fall
                pPlayer->PlayDirectSound(OutdoorPvP_WG_SOUND_START_BATTLE);     // START Battle
                pPlayer->CastSpell(pPlayer, SPELL_RECRUIT, true);
                if(SpellAuraHolderPtr pHolder = pPlayer->GetSpellAuraHolder(SPELL_TOWER_CONTROL))
                    pHolder->SetStackAmount(3);
            }
        }


        if(pPlayer->GetTeam() == GetDefender())
        {
            HandlePlayerRemoveAuras();
            pPlayer->RemoveAurasDueToSpell(SPELL_SPIRITUAL_IMMUNITY);
            if (pPlayer->getLevel() < 75)
                pPlayer->CastSpell(pPlayer, SPELL_TELEPORT_DALARAN, true);
            else
            {
                //CountDef++;
                pPlayer->RemoveAurasDueToSpell(SPELL_AURA_FLY);
                pPlayer->CastSpell(pPlayer, SPELL_PARAHUTE, true); //prevent die if fall
                pPlayer->PlayDirectSound(OutdoorPvP_WG_SOUND_START_BATTLE); // START Battle
                pPlayer->CastSpell(pPlayer, SPELL_RECRUIT, true);
            }
        }
    }


    // battle starts
    m_bIsBattleStarted = true;
    sLog.outDebug("=== Wintergrasp battle has started! ===");
}

void WorldPvPWG::EndBattle()
{
    // battle ends
    
    // stuff related to the end of battle
    
    // cleanup and preparing for next battle
    InitBattlefield();
}

void WorldPvPWG::ProcessEvent(GameObject* pGo, uint32 uiEventId, uint32 uiFaction)
{
}

void WorldPvPWG::FillInitialWorldStates(WorldPacket& data, uint32& count)
{
}

void WorldPvPWG::SendRemoveWorldStates(Player* pPlayer)
{
}

void WorldPvPWG::ProcessCaptureEvent(uint32 uiCaptureType, uint32 uiTeam, uint32 uiNewWorldState, uint32 uiFactor)
{
}

void WorldPvPWG::HandlePlayerEnterZone(Player* pPlayer)
{
    if (m_bIsWarTime())
    {
        if(pPlayer->getLevel() < 75)
        {
            pPlayer->CastSpell(pPlayer, SPELL_TELEPORT_DALARAN, true);
            return;
        }
        if(pPlayer->getLevel() > 74)
        {
            if (!pPlayer->HasAura(SPELL_RECRUIT) && !pPlayer->HasAura(SPELL_CORPORAL)
                && !pPlayer->HasAura(SPELL_LIEUTENANT))
            pPlayer->CastSpell(pPlayer, SPELL_RECRUIT, true);
        }
        else if(pPlayer->GetBGTeam() == GetAttacker())
        {
            if (m_towerDestroyedCount[GetAttacker()] < 3)
            {
                pPlayer->CastSpell(pPlayer, SPELL_TOWER_CONTROL, true);
                //maby need add HasAura(SPELL_TOWER_CONTROL)
                if(SpellAuraHolderPtr pHolder = pPlayer->GetSpellAuraHolder(SPELL_TOWER_CONTROL))
                {
                    pHolder->SetStackAmount(3 - m_towerDestroyedCount[GetAttacker()]);
                }
                //plr->SetAuraStack(SPELL_TOWER_CONTROL, plr, 3 - m_towerDestroyedCount[GetAttackerTeam()]);
            }
            else if (m_towerDestroyedCount[GetAttacker()])
            {
                //plr->SetAuraStack(SPELL_TOWER_CONTROL, plr, m_towerDestroyedCount[GetAttacker()]);
            }
        }
    }

    HandlePlayerRemoveAuras();
    WorldPvP::HandlePlayerEnterZone(pPlayer);
}

void WorldPvPWG::HandlePlayerLeaveZone(Player* pPlayer)
{
    HandlePlayerRemoveAuras();
    WorldPvP::HandlePlayerLeaveZone(pPlayer);
}

void WorldPvPWG::HandlePlayerRemoveAuras()
{
    Player* pPlayer = GetPlayerInZone();
    if (!pPlayer)
        return;

    // remove the buff from the player first; Sometimes on relog players still have the aura
    if (pPlayer->HasAura(SPELL_RECRUIT) || pPlayer->HasAura(SPELL_CORPORAL) 
        || pPlayer->HasAura(SPELL_LIEUTENANT) || pPlayer->HasAura(SPELL_TOWER_CONTROL))
    {   
        pPlayer->RemoveAurasDueToSpell(SPELL_RECRUIT);
        pPlayer->RemoveAurasDueToSpell(SPELL_CORPORAL);
        pPlayer->RemoveAurasDueToSpell(SPELL_LIEUTENANT);
        pPlayer->RemoveAurasDueToSpell(SPELL_TOWER_CONTROL);
    }
}

void WorldPvPWG::OnCreatureCreate(Creature* pCreature)
{
    switch(pCreature->GetEntry())
    {
        case NPC_TOWER_CANNON:
        {
            if (GetDefender() == ALLIANCE)
            {
                pCreature->setFaction(WG_VEHICLE_FACTION_ALLIANCE);
            }
            else if (GetDefender() == HORDE)
            {                
                pCreature->setFaction(WG_VEHICLE_FACTION_HORDE);
            }
        }
    }
}

void WorldPvPWG::OnGameObjectCreate(GameObject* pGo)
{
    switch (pGo->GetEntry())
    {
        case GO_FACTORY_BANNER_NE:
        case GO_FACTORY_BANNER_NW:
        case GO_FACTORY_BANNER_SE:
        case GO_FACTORY_BANNER_SW:
            pGo->SetGoArtKit(GO_ARTKIT_BANNER_ALLIANCE);
            break;
        case GO_WG_DOOR:
        case GO_WG_DOOR_1:
        case GO_WG_TOWER:
        case GO_WG_WALL_1:
        case GO_WG_WALL_2:
        case GO_WG_WALL_3:
        case GO_WG_WALL_4:
        case GO_WG_WALL_5:
        case GO_WG_WALL_6:
        case GO_WG_WALL_7:
        case GO_WG_WALL_8:
        case GO_WG_WALL_9:
        case GO_WG_WALL_10:
        case GO_WG_WALL_11:
        case GO_WG_WALL_12:
        case GO_WG_WALL_13:
        case GO_WG_WALL_14:
            if (pGo->GetGOInfo()->type == GAMEOBJECT_TYPE_DESTRUCTIBLE_BUILDING)
                pGo->Rebuild(NULL);
            pGo->SetGoArtKit(m_uiZoneController == ALLIANCE ? GO_ARTKIT_BANNER_ALLIANCE : GO_ARTKIT_BANNER_HORDE);
            break;
    }
}


void WorldPvPWG::SetBannerArtKit(ObjectGuid BannerGuid, uint32 uiArtkit)
{
    // neet to use a player as anchor for the map
    Player* pPlayer = GetPlayerInZone();
    if (!pPlayer)
        return;

    if (GameObject* pBanner = pPlayer->GetMap()->GetGameObject(BannerGuid))
    {
        pBanner->SetGoArtKit(uiArtkit);
        pBanner->Refresh();
    }
}


/*####################################################
################ UPDATE_BATTLEGROUND #################
####################################################*/

void WorldPvPWG::Update(uint32 uiDiff)
{
    // if battle not started
    if (!m_bIsBattleStarted)
    {
        // time to start
        if (m_uiNextBattleStartTimer < uiDiff)
        {
            StartBattle();
        }
        else
            m_uiNextBattleStartTimer -= uiDiff;
    }
    else
    {
        // battle in proggress
    }    
}

void WorldPvPWG::OnBattleground(GameObject* pGo, Player* pPlayer)
{
    switch (pGo->GetEntry())
    {
        case GO_WG_TOWER:
        {
            if(pGo->HasFlag(GAMEOBJECT_FLAGS, GO_FLAG_DESTROYED))
            {
                if(m_uiDestroyedTower <= 2)
                {   
                    if(SpellAuraHolderPtr pHolder = pPlayer->GetSpellAuraHolder(SPELL_TOWER_CONTROL))
                    {
                        if (pPlayer->GetTeam() != GetDefender())
                        {
                            pHolder->ModStackAmount(-1);
                        }
                        else if (pPlayer->GetTeam() == GetDefender())
                        {
                            pPlayer->CastSpell(pPlayer, SPELL_TOWER_CONTROL, true);
                        }
                    }
                    else if(m_uiDestroyedTower >= 3)
                    {
                        if (pPlayer->GetTeam() != GetDefender())
                        {
                            pHolder->ModStackAmount(-1);
                        }
                        else if (pPlayer->GetTeam() == GetDefender())
                        {
                            pPlayer->CastSpell(pPlayer, SPELL_TOWER_CONTROL, true);
                        }

                        m_uiBattlegroundIsStarted -= 10*MINUTE;
                    }
                }
            }
        }
    }
}

void WorldPvPWG::HandlePlayerKillInsideArea(Player* pPlayer, Unit* pVictim)
{
    if(m_bIsWarTime())
    {
        if(pPlayer->HasAura(SPELL_RECRUIT))
        {
            if(SpellAuraHolderPtr pHolder = pPlayer->GetSpellAuraHolder(SPELL_RECRUIT))
            {
                if (pHolder->GetStackAmount() >= 5)
                {
                    pPlayer->RemoveAurasDueToSpell(SPELL_RECRUIT);
                    pPlayer->CastSpell(pPlayer, SPELL_CORPORAL, true);
                    ChatHandler(pPlayer).PSendSysMessage(LANG_BG_WG_RANK1);
                } else pPlayer->CastSpell(pPlayer, SPELL_RECRUIT, true);
            }
            else if (pPlayer->HasAura(SPELL_CORPORAL))
            {
                if(SpellAuraHolderPtr pHolder = pPlayer->GetSpellAuraHolder(SPELL_CORPORAL))
                {
                    if (pHolder->GetStackAmount() >= 5)
                    {
                        pPlayer->RemoveAurasDueToSpell(SPELL_CORPORAL);
                        pPlayer->CastSpell(pPlayer, SPELL_LIEUTENANT, true);
                        ChatHandler(pPlayer).PSendSysMessage(LANG_BG_WG_RANK2);
                    } else pPlayer->CastSpell(pPlayer, SPELL_CORPORAL, true);
                }
            }
        }
    }
}

/*####################################################
################ END_BATTLEGROUND_SYSTEM #############
####################################################*/

bool WorldPvPWG::HandleObjectUse(Player* pPlayer, GameObject* pGo)
{
    if(pGo->GetEntry() == GO_TITAN_RELIC)
    {
        if (pPlayer->GetTeam() != GetDefender())
        {
            //If attacker win defender dont control zone
            EndBattle();
        }
    }

    return true;
}

void WorldPvPWG::EndBattleGround(Player* pPlayer, Team winner, GameObject* pGo)
{
    if (winner != GetDefender())
    {
        if(pPlayer->HasAura(SPELL_CORPORAL))
        {
            // 2 Marks of Wg
        }
        if(pPlayer->HasAura(SPELL_LIEUTENANT))
        {
            // 3 Marks of Wg
        }
    }
    else if(pPlayer->HasAura(SPELL_RECRUIT))
    {
        // 1 Marks of Wg
    }

    //Repair destroyed buildings and setgoartkit
    switch (pGo->GetEntry())
    {
        case GO_FACTORY_BANNER_NE:
        case GO_FACTORY_BANNER_NW:
        case GO_FACTORY_BANNER_SE:
        case GO_FACTORY_BANNER_SW:
            pGo->SetGoArtKit(m_uiZoneController == ALLIANCE ? GO_ARTKIT_BANNER_ALLIANCE : GO_ARTKIT_BANNER_HORDE);
            break;
        case GO_WG_DOOR:
        case GO_WG_DOOR_1:
        case GO_WG_TOWER:
        case GO_WG_WALL_1:
        case GO_WG_WALL_2:
        case GO_WG_WALL_3:
        case GO_WG_WALL_4:
        case GO_WG_WALL_5:
        case GO_WG_WALL_6:
        case GO_WG_WALL_7:
        case GO_WG_WALL_8:
        case GO_WG_WALL_9:
        case GO_WG_WALL_10:
        case GO_WG_WALL_11:
        case GO_WG_WALL_12:
        case GO_WG_WALL_13:
        case GO_WG_WALL_14:
            if (pGo->GetGOInfo()->type == GAMEOBJECT_TYPE_DESTRUCTIBLE_BUILDING)
                pGo->Rebuild(NULL);
            pGo->SetGoArtKit(m_uiZoneController == ALLIANCE ? GO_ARTKIT_BANNER_ALLIANCE : GO_ARTKIT_BANNER_HORDE);
            break;
    }
}

/*####################################################
################### INVITE_SYSTEM ####################
####################################################*/
