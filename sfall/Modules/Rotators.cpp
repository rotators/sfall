#include <string>
#include <vector>

#include "..\main.h"
#include "..\SafeWrite.h"

#include "Rotators.h"

// Remember to wear protective goggles :)

const char rotatorsIni[] = ".\\ddraw.rotators.ini";

static void InitCustomDll()
{
    std::vector<std::string> names = sfall::GetIniList( "Debugging", "CustomDll", "", 512, ',', rotatorsIni );

    for( const auto& name : names )
    {
        if( name.empty() )
            continue;

        sfall::dlog_f( "Loading custom dll %s", DL_MAIN, name.c_str() );

        LoadLibraryA( name.c_str() );
    }
}

void sfall::Rotators::init()
{
    InitCustomDll();
}

void sfall::Rotators::exit()
{
}
