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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/// \addtogroup mangosd Mangos Daemon
/// @{
/// \file

#include "Common.h"
#include "Database/DatabaseEnv.h"
#include "Config/Config.h"
#include "ProgressBar.h"
#include "Log.h"
#include "Master.h"
#include "SystemConfig.h"
#include "AuctionHouseBot/AuctionHouseBot.h"
#include "revision.h"
#include "revision_nr.h"
#include "revision_R2.h"
#include <openssl/opensslv.h>
#include <openssl/crypto.h>
#include <ace/Version.h>
#include <ace/Get_Opt.h>

#ifdef WIN32
#include "ServiceWin32.h"
char serviceName[] = "mangosd";
char serviceLongName[] = "MaNGOS world service";
char serviceDescription[] = "Massive Network Game Object Server";
/*
 * -1 - not in service mode
 *  0 - stopped
 *  1 - running
 *  2 - paused
 */
int m_ServiceStatus = -1;
#else
#include "PosixDaemon.h"
#endif

DatabaseType WorldDatabase;                                 ///< Accessor to the world database
DatabaseType CharacterDatabase;                             ///< Accessor to the character database
DatabaseType LoginDatabase;                                 ///< Accessor to the realm/login database

uint32 realmID;                                             ///< Id of the realm

/// Print out the usage string for this program on the console.
void usage(const char *prog)
{
    sLog.outString("Usage: \n %s [<options>]\n"
        "    -v, --version            print version and exist\n\r"
        "    -c config_file           use config_file as configuration file\n\r"
        "    -a, --ahbot config_file  use config_file as ahbot configuration file\n\r"
        #ifdef WIN32
        "    Running as service functions:\n\r"
        "    -s run                   run as service\n\r"
        "    -s install               install service\n\r"
        "    -s uninstall             uninstall service\n\r"
        #else
        "    Running as daemon functions:\n\r"
        "    -s run                   run as daemon\n\r"
        "    -s stop                  stop daemon\n\r"
        #endif
        ,prog);
}

/// Launch the mangos server
extern int main(int argc, char **argv)
{
    ///- Command line parsing
    char const* cfg_file = _MANGOSD_CONFIG;


    char const *options = ":a:c:s:";

    ACE_Get_Opt cmd_opts(argc, argv, options);
    cmd_opts.long_option("version", 'v', ACE_Get_Opt::NO_ARG);
    cmd_opts.long_option("ahbot", 'a', ACE_Get_Opt::ARG_REQUIRED);

    char serviceDaemonMode = '\0';

    int option;
    while ((option = cmd_opts()) != EOF)
    {
        switch (option)
        {
            case 'a':
                sAuctionBotConfig.SetConfigFileName(cmd_opts.opt_arg());
                break;
            case 'c':
                cfg_file = cmd_opts.opt_arg();
                break;
            case 'v':
                printf("%s\n", _FULLVERSION(REVISION_NR));
                printf("%s\n", _R2FULLVERSION(REVISION_DATE,REVISION_TIME,REVISION_R2,REVISION_ID));
                return 0;
            case 's':
            {
                const char *mode = cmd_opts.opt_arg();

                if (!strcmp(mode, "run"))
                    serviceDaemonMode = 'r';
#ifdef WIN32
                else if (!strcmp(mode, "install"))
                    serviceDaemonMode = 'i';
                else if (!strcmp(mode, "uninstall"))
                    serviceDaemonMode = 'u';
#else
                else if (!strcmp(mode, "stop"))
                    serviceDaemonMode = 's';
#endif
                else
                {
                    sLog.outError("Runtime-Error: -%c unsupported argument %s", cmd_opts.opt_opt(), mode);
                    usage(argv[0]);
                    Log::WaitBeforeContinueIfNeed();
                    return 1;
                }
                break;
            }
            case ':':
                sLog.outError("Runtime-Error: -%c option requires an input argument", cmd_opts.opt_opt());
                usage(argv[0]);
                Log::WaitBeforeContinueIfNeed();
                return 1;
            default:
                sLog.outError("Runtime-Error: bad format of commandline arguments");
                usage(argv[0]);
                Log::WaitBeforeContinueIfNeed();
                return 1;
        }
    }

#ifdef WIN32                                                // windows service command need execute before config read
    switch (serviceDaemonMode)
    {
        case 'i':
            if (WinServiceInstall())
                sLog.outString("Installing service");
            return 1;
        case 'u':
            if (WinServiceUninstall())
                sLog.outString("Uninstalling service");
            return 1;
        case 'r':
            WinServiceRun();
            break;
    }
#endif

    if (!sConfig.SetSource(cfg_file))
    {
        sLog.outError("找不到配置文件 %s。请确认您的配置文件是否配置正确！", cfg_file);
        Log::WaitBeforeContinueIfNeed();
        return 1;
    }

#ifndef WIN32                                               // posix daemon commands need apply after config read
    switch (serviceDaemonMode)
    {
    case 'r':
        startDaemon();
        break;
    case 's':
        stopDaemon();
        break;
    }
#endif

    sLog.outString( "初始化……");
    sLog.outString( "==============================================================");
    sLog.outString( "欢迎使用 ChgMangos!              Chglove 2011.10.10"           );
    sLog.outString( "使用 [Ctrl+C] 关闭服务器"                                      );
    sLog.outString();
    sLog.outString( "              C H G M A N G O S - P R O J E C T               ");    
    sLog.outString( "                    Enjoy your WOW Word!                      ");    
    sLog.outString( ".:::::: :::   ::: .::::::::  :::       ::: .:::::::: .::::::::");
    sLog.outString( ":+      :+:   +:+ :+:        +: +     + +# :+:       :#:      ");
    sLog.outString( "+#      #++##++#+ +:+    ### +#  +   #  #+ +:+    ### +#++#++.");
    sLog.outString( "#+      +#+   +#+ +#+     #  +#   # #   ## +#+     #       +#+");
    sLog.outString( " ###### ###   ###  ########  ##    #    ##  ######## #########");
    sLog.outString( "==============================================================");
    sLog.outString( "ChgMangos_WoTLK_1.0                      2011.10.10"           );
    sLog.outString( "Mangos 11816, SD2 2307, ChgMDB_WoTLK_final_1.0_ACID 3.0.8     ");
    sLog.outString( "==============================================================");
    sLog.outString();
    sLog.outString("游戏服务器启动开始……");
    sLog.outString( "==============================================================");
    sLog.outString("加载配置文件 %s……", cfg_file);
    sLog.outString();

    DETAIL_LOG("%s （库文件版本： %s）", OPENSSL_VERSION_TEXT, SSLeay_version(SSLEAY_VERSION));
    if (SSLeay() < 0x009080bfL )
    {
        DETAIL_LOG("警告： OpenSSL lib 版本过低， 登陆服务器将无法正常工作！ ");
        DETAIL_LOG("警告： 最低版本需求  [OpenSSL 0.9.8k]");
    }

    DETAIL_LOG("ACE 版本： %s", ACE_VERSION);

    ///- Set progress bars show mode
    BarGoLink::SetOutputState(sConfig.GetBoolDefault("ShowProgressBars", true));

    ///- and run the 'Master'
    /// \todo Why do we need this 'Master'? Can't all of this be in the Main as for Realmd?
    return sMaster.Run();

    // at sMaster return function exist with codes
    // 0 - normal shutdown
    // 1 - shutdown at error
    // 2 - restart command used, this code can be used by restarter for restart mangosd
}

/// @}
