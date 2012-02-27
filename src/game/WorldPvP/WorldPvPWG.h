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


#ifndef WORLD_PVP_WG
#define WORLD_PVP_WG

enum 
{
    // zone ids
    ZONE_ID_WINTERGRASP                 = 4197,
    MAX_WG_FACTORS                      = 4,    

    // misc

    // events

    // map world states
};

enum WorldStates
{
    WORLD_STATE_WG_SHOW_START_TIMER             = 3801,
    WORLD_STATE_WG_START_TIMER                  = 4354,

    WORLD_STATE_VEHICLE_H         = 3490,
    WORLD_STATE_MAX_VEHICLE_H     = 3491,
    WORLD_STATE_VEHICLE_A         = 3680,
    WORLD_STATE_MAX_VEHICLE_A     = 3681,
    WORLD_STATE_ACTIVE            = 3801,
    WORLD_STATE_DEFENDER          = 3802,
    WORLD_STATE_ATTACKER          = 3803,
};

enum Spells
{
    SPELL_RECRUIT                               = 37795,  //If player enter the zone
    SPELL_CORPORAL                              = 33280,  //Can spawn Catapult 
    SPELL_LIEUTENANT                            = 55629,  //Can spawn all vehicle
    SPELL_TENACITY                              = 58549,
    SPELL_TENACITY_VEHICLE                      = 59911,
    SPELL_TOWER_CONTROL                         = 62064,  //Incrase dmg for 5% when tower is destruction
    SPELL_SPIRITUAL_IMMUNITY                    = 58729,
    SPELL_GREAT_HONOR                           = 58555,
    SPELL_GREATER_HONOR                         = 58556,
    SPELL_GREATEST_HONOR                        = 58557,
    SPELL_ALLIANCE_FLAG                         = 14268,
    SPELL_HORDE_FLAG                            = 14267,

    // Reward spells
    SPELL_VICTORY_REWARD                        = 56902,
    SPELL_DEFEAT_REWARD                         = 58494,
    SPELL_DAMAGED_TOWER                         = 59135,
    SPELL_DESTROYED_TOWER                       = 59136,
    SPELL_DAMAGED_BUILDING                      = 59201,
    SPELL_INTACT_BUILDING                       = 59203,

    SPELL_TELEPORT_BRIDGE                       = 59096,
    SPELL_TELEPORT_FORTRESS                     = 60035,

    SPELL_TELEPORT_DALARAN                      = 53360,
    SPELL_VICTORY_AURA                          = 60044,

    // Other spells
    SPELL_WINTERGRASP_WATER                     = 36444,
    SPELL_ESSENCE_OF_WINTERGRASP                = 58045,
    SPELL_WINTERGRASP_RESTRICTED_FLIGHT_AREA    = 58730,
    SPELL_PARAHUTE                              = 45472,

    // Phasing spells
    SPELL_HORDE_CONTROLS_FACTORY_PHASE_SHIFT    = 56618,// ADDS PHASE 16
    SPELL_ALLIANCE_CONTROLS_FACTORY_PHASE_SHIFT = 56617,// ADDS PHASE 32

    SPELL_HORDE_CONTROL_PHASE_SHIFT             = 55773,// ADDS PHASE 64
    SPELL_ALLIANCE_CONTROL_PHASE_SHIFT          = 55774,// ADDS PHASE 128
};

enum GameObjectId
{
    GO_WG_DOOR                                  = 191810,    //Wintergrasp Fortress Door
    GO_WG_DOOR_1                                = 190375,    //Wintergrasp Fortress Gate

    GO_WG_TOWER                                 = 190357,   //Winter's Edge Tower

    GO_WG_WALL_1                                = 191802,   //Wintergrasp Fortress Wall
    GO_WG_WALL_2                                = 191803,
    GO_WG_WALL_3                                = 191795, 
    GO_WG_WALL_4                                = 191804,
    GO_WG_WALL_5                                = 191800,
    GO_WG_WALL_6                                = 191796,
    GO_WG_WALL_7                                = 191807,
    GO_WG_WALL_8                                = 191801,
    GO_WG_WALL_9                                = 191808,
    GO_WG_WALL_10                               = 191806,
    GO_WG_WALL_11                               = 191809,
    GO_WG_WALL_12                               = 191799,

    GO_WG_WALL_13                               = 191798,   //Wintergrasp Wall
    GO_WG_WALL_14                               = 191805,

    GO_FACTORY_BANNER_NE                        = 190475,
    GO_FACTORY_BANNER_NW                        = 190487,
    GO_FACTORY_BANNER_SE                        = 194959,
    GO_FACTORY_BANNER_SW                        = 194962,

    GO_TITAN_RELIC                              = 192829,

};          

enum GateStatus
{
    GO_GATES_NORMAL                             = 1,
    GO_GATES_DAMAGE                             = 2,
    GO_GATES_DESTROY                            = 3,
};

enum Faction
{
    WG_VEHICLE_FACTION_NEUTRAL = 35,
    WG_VEHICLE_FACTION_ALLIANCE = 3,
    WG_VEHICLE_FACTION_HORDE = 6
};

enum CreatureId
{
    NPC_GUARD_H                                 = 30739,
    NPC_GUARD_A                                 = 30740,
    NPC_STALKER                                 = 00000,

    NPC_VIERON_BLAZEFEATHER                     = 31102,
    NPC_STONE_GUARD_MUKAR                       = 32296,// <WINTERGRASP QUARTERMASTER>
    NPC_HOODOO_MASTER_FU_JIN                    = 31101,// <MASTER HEXXER>
    NPC_CHAMPION_ROS_SLAI                       = 39173,// <WINTERGRASP QUARTERMASTER>
    NPC_COMMANDER_DARDOSH                       = 31091,
    NPC_TACTICAL_OFFICER_KILRATH                = 31151,
    NPC_SIEGESMITH_STRONGHOOF                   = 31106,
    NPC_PRIMALIST_MULFORT                       = 31053,
    NPC_LIEUTENANT_MURP                         = 31107,

    NPC_BOWYER_RANDOLPH                         = 31052,
    NPC_KNIGHT_DAMERON                          = 32294,// <WINTERGRASP QUARTERMASTER>
    NPC_SORCERESS_KAYLANA                       = 31051,// <ENCHANTRESS>
    NPC_MARSHAL_MAGRUDER                        = 39172,// <WINTERGRASP QUARTERMASTER>
    NPC_COMMANDER_ZANNETH                       = 31036,
    NPC_TACTICAL_OFFICER_AHBRAMIS               = 31153,
    NPC_SIEGE_MASTER_STOUTHANDLE                = 31108,
    NPC_ANCHORITE_TESSA                         = 31054,
    NPC_SENIOR_DEMOLITIONIST_LEGOSO             = 31109,

    NPC_TAUNKA_SPIRIT_GUIDE                     = 31841,    // Horde spirit guide for Wintergrasp
    NPC_DWARVEN_SPIRIT_GUIDE                    = 31842,    // Alliance spirit guide for Wintergrasp

    NPC_TOWER_CANNON                            = 28366,    //Cannon in tower
};

enum Sounds
{
    OutdoorPvP_WG_SOUND_KEEP_CLAIMED            = 8192,
    OutdoorPvP_WG_SOUND_KEEP_CAPTURED_ALLIANCE  = 8173,
    OutdoorPvP_WG_SOUND_KEEP_CAPTURED_HORDE     = 8213,
    OutdoorPvP_WG_SOUND_KEEP_ASSAULTED_ALLIANCE = 8212,
    OutdoorPvP_WG_SOUND_KEEP_ASSAULTED_HORDE    = 8174,
    OutdoorPvP_WG_SOUND_NEAR_VICTORY            = 8456,
    OutdoorPvP_WG_SOUND_HORDE_WINS              = 8454,
    OutdoorPvP_WG_SOUND_ALLIANCE_WINS           = 8455,
    OutdoorPvP_WG_SOUND_WORKSHOP_Horde          = 6205,
    OutdoorPvP_WG_SOUND_WORKSHOP_ALLIANCE       = 6298,
    OutdoorPvP_WG_HORDE_CAPTAIN                 = 8333,
    OutdoorPvP_WG_ALLIANCE_CAPTAIN              = 8232,
    OutdoorPvP_WG_SOUND_START_BATTLE            = 3439, //Standart BG Start sound
};

static const uint32 aWintergraspFactors[MAX_WG_FACTORS] = {GO_FACTORY_BANNER_NE, GO_FACTORY_BANNER_NW, GO_FACTORY_BANNER_SE, GO_FACTORY_BANNER_SW};

class WorldPvPWG : public WorldPvP
{
    public:
        WorldPvPWG();

        bool InitWorldPvPArea();

        void OnCreatureCreate(Creature* pCreature);
        void OnGameObjectCreate(GameObject* pGo);
        void ProcessEvent(GameObject* pGo, uint32 uiEventId, uint32 uiFaction);

        virtual void Update(uint32 uiDiff);

        void HandlePlayerEnterZone(Player* pPlayer);
        void HandlePlayerLeaveZone(Player* pPlayer);
        void HandlePlayerKillInsideArea(Player* pPlayer, Unit* pVictim);
        
        void EndBattleGround(Player* pPlayer, Team winner, GameObject* pGo);
        void OnBattleground(GameObject* pGo, Player* pPlayer);

        void FillInitialWorldStates(WorldPacket& data, uint32& count);
        void SendRemoveWorldStates(Player* pPlayer);

        bool HandleObjectUse(Player* pPlayer, GameObject* pGo);

        Team GetDefender() const { return defender; }
        Team GetAttacker() const { return attacker; }

        Team defender;
        Team attacker;

    protected:
        bool m_bIsWarTime() { return m_bIsBattleStarted; }

        uint32 m_towerDestroyedCount[2];
        uint32 m_towerDamagedCount[2];

    private:
        void ProcessCaptureEvent(uint32 uiCaptureType, uint32 uiTeam, uint32 uiNewWorldState, uint32 uiFactor);
        void SetBannerArtKit(ObjectGuid BannerGuid, uint32 uiArtkit);
        bool m_bIsBattleStarted;

        void InitBattlefield();
        void StartBattle();
        void EndBattle();
        void HandlePlayerRemoveAuras();

        uint32 m_uiZoneController;
        uint32 m_uiDestroyedTower;

        uint32 m_uiNextBattleStartTimer;
        uint32 m_uiBattlegroundIsStarted;

};
#endif
