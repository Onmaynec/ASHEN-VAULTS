#include "AVContent.h"

float FAVContent::HeroHealth(int32 Id) { return Id == 1 ? 82.f : Id == 2 ? 75.f : Id == 3 ? 90.f : 115.f; }
float FAVContent::HeroStamina(int32 Id) { return Id == 1 ? 120.f : Id == 2 ? 135.f : Id == 3 ? 115.f : 100.f; }
float FAVContent::HeroAttack(int32 Id) { return Id == 1 ? 22.f : Id == 2 ? 16.f : Id == 3 ? 19.f : 18.f; }
float FAVContent::HeroSpeed(int32 Id) { return Id == 1 ? 5.7f : Id == 2 ? 4.6f : Id == 3 ? 5.1f : 4.8f; }

int32 FAVContent::EnemyLevel(int32 Floor, int32 Roll)
{
    const int32 Depth = Floor > 1 ? Floor - 1 : 0;
    const int32 Variation = Roll < 0 ? 0 : Roll > 99 ? 99 : Roll;
    return 1 + Depth * 2 + Variation / 34;
}
float FAVContent::LevelHealthMultiplier(int32 Level)
{
    return 1.f + 0.13f * (Level > 1 ? Level - 1 : 0);
}
float FAVContent::LevelDamageMultiplier(int32 Level)
{
    return 1.f + 0.065f * (Level > 1 ? Level - 1 : 0);
}

float FAVContent::ScaledEnemyHealth(int32 Type, int32 Floor, int32 Level)
{
    // Enemy level is already based on absolute depth; a second floor
    // multiplier made health grow quadratically and eventually stalled runs.
    return Enemy(Type, Floor).Health * LevelHealthMultiplier(Level);
}

int32 FAVContent::BanditGroupSize(int32 Floor)
{
    // Keep the hostile camp a distinct, demanding encounter without
    // spawning dozens of units in a single chamber in late cycles.
    const int32 Depth = Floor > 1 ? Floor - 1 : 0;
    return 3 + FMath::Min(5, Depth / 4);
}

int32 FAVContent::Biome(int32 Floor)
{
    return ((FMath::Max(1, Floor) - 1) / FloorsPerBiome) % BiomeCount;
}

int32 FAVContent::Boss(int32 Floor)
{
    if (Floor < 1 || Floor % 5 != 0) return -1;
    return Biome(Floor) == 0 ? 8 : Biome(Floor) == 1 ? 9 : Biome(Floor) == 2 ? 3 : 56;
}

int32 FAVContent::EnemyRoll(int32 Floor, int32 Percentile)
{
    const int32 P = Percentile < 0 ? 0 : (Percentile > 99 ? 99 : Percentile);
    switch (Biome(Floor))
    {
    case 0: return P < 17 ? 0 : P < 30 ? 15 : P < 43 ? 16 : P < 55 ? 17 : P < 65 ? 18 : P < 74 ? 19 : P < 82 ? 20 : P < 89 ? 1 : P < 95 ? 11 : 5;
    case 1: return P < 17 ? 5 : P < 31 ? 6 : P < 44 ? 21 : P < 57 ? 22 : P < 69 ? 23 : P < 79 ? 12 : P < 88 ? 2 : P < 94 ? 16 : 1;
    case 2: return P < 18 ? 7 : P < 33 ? 10 : P < 47 ? 24 : P < 59 ? 22 : P < 72 ? 13 : P < 83 ? 14 : P < 91 ? 23 : 1;
    default: return 34 + P * 18 / 100; // Black Bastion: all 18 archetypes
    }
}

FAVEnemySpec FAVContent::Enemy(int32 Type, int32 Floor)
{
    FAVEnemySpec S = { 30.f, 2.2f, 12.f, false, false };
    switch (Type)
    {
    case 0: S = { 29.f, 2.45f, 12.f, false, false }; break;
    case 1: S = { 46.f, 1.65f, 18.f, false, false }; break;
    case 2: S = { 22.f, 2.05f, 15.f, true, false }; break;
    case 3: S = { 520.f, 1.8f, 26.f, true, true }; break;
    case 4: S = { 62.f, 3.05f, 19.f, false, false }; break;
    case 5: S = { 25.f, 3.4f, 14.f, false, false }; break;
    case 6: S = { 31.f, 1.75f, 12.f, true, false }; break;
    case 7: S = { 80.f, 1.28f, 23.f, false, false }; break;
    case 8: S = { 240.f, 1.65f, 20.f, false, true }; break;
    case 9: S = { 360.f, 1.52f, 23.f, true, true }; break;
    case 10: S = { 21.f, 2.85f, 13.f, true, false }; break;
    case 11: S = { 34.f, 3.10f, 16.f, false, false }; break; // Ash Reaver
    case 12: S = { 40.f, 1.85f, 17.f, true, false }; break; // Spore Witch
    case 13: S = { 64.f, 2.25f, 21.f, false, false }; break; // Ember Knight
    case 14: S = { 27.f, 3.35f, 16.f, true, false }; break; // Cinder Specter
    case 15: S = { 19.f, 3.7f, 7.f, false, false }; break; // Rat
    case 16: S = { 38.f, 1.8f, 12.f, false, false }; break; // Zombie
    case 17: S = { 29.f, 2.4f, 13.f, false, false }; break; // Skeleton
    case 18: S = { 23.f, 2.8f, 14.f, true, false }; break; // Ghost
    case 19: S = { 32.f, 1.9f, 16.f, true, false }; break; // Witch
    case 20: S = { 50.f, 1.55f, 18.f, false, false }; break; // Crypt Guard
    case 21: S = { 35.f, 2.1f, 14.f, true, false }; break; // Druid
    case 22: S = { 24.f, 3.1f, 12.f, false, false }; break; // Wretch
    case 23: S = { 95.f, 1.15f, 24.f, false, false }; break; // Stone Golem
    case 24: S = { 120.f, 1.05f, 27.f, false, false }; break; // Iron Golem
    case 25: S = { 42.f, 2.7f, 17.f, true, false }; break; // Bandit Archer
    case 26: S = { 175.f, 2.1f, 23.f, false, false }; break; // Grave Butcher
    case 27: S = { 145.f, 2.5f, 20.f, true, false }; break; // Bone Archer
    case 28: S = { 190.f, 1.6f, 22.f, true, false }; break; // Rot Witch
    case 29: S = { 230.f, 1.3f, 25.f, false, false }; break; // Stone Sentinel
    case 30: S = { 165.f, 2.6f, 22.f, false, false }; break; // Blood Reaver
    case 31: S = { 205.f, 1.9f, 24.f, true, false }; break; // Cinder Shaman
    case 32: S = { 250.f, 1.5f, 28.f, true, false }; break; // Iron Tyrant
    case 33: S = { 45.f, 2.65f, 17.f, false, false }; break; // Bandit Raider
    // Black Bastion (34..51), four minibosses (52..55), Commandant (56).
    case 34: S = {46.f,2.2f,17.f,false,false}; break; // Black Infantry
    case 35: S = {42.f,1.9f,18.f,false,false}; break; // Pikeman
    case 36: S = {82.f,1.25f,21.f,false,false}; break; // Shieldbearer
    case 37: S = {37.f,1.55f,18.f,true,false}; break; // Crossbowman
    case 38: S = {63.f,1.45f,22.f,false,false}; break; // Halberdier
    case 39: S = {50.f,2.9f,20.f,false,false}; break; // Berserker
    case 40: S = {45.f,3.35f,16.f,false,false}; break; // Armored Hound
    case 41: S = {50.f,2.7f,18.f,false,false}; break; // Gargoyle
    case 42: S = {110.f,1.02f,27.f,false,false}; break; // Bonebreaker
    case 43: S = {28.f,3.3f,14.f,false,false}; break; // Crawler
    case 44: S = {88.f,1.85f,23.f,false,false}; break; // Possessed Knight
    case 45: S = {49.f,2.7f,20.f,true,false}; break; // Dark Hunter
    case 46: S = {44.f,1.7f,21.f,true,false}; break; // Battle Mage
    case 47: S = {48.f,1.5f,16.f,true,false}; break; // Summoner
    case 48: S = {96.f,1.35f,26.f,false,false}; break; // Punisher
    case 49: S = {52.f,1.6f,17.f,true,false}; break; // Blood Priest
    case 50: S = {87.f,1.42f,23.f,true,false}; break; // Seal Sentinel
    case 51: S = {44.f,3.0f,20.f,false,false}; break; // Ravager
    case 52: S = {200.f,1.9f,32.f,false,false}; break; // Executioner
    case 53: S = {220.f,1.6f,34.f,false,false}; break; // Guard Captain
    case 54: S = {290.f,1.05f,40.f,false,false}; break; // Siege Golem
    case 55: S = {190.f,2.2f,32.f,true,false}; break; // Inquisitor
    case 56: S = {650.f,1.7f,42.f,false,true}; break; // Commandant
    default: break;
    }
    // Health scaling belongs to ScaledEnemyHealth(), not the base spec.
    (void)Floor;
    return S;
}

const char* FAVContent::BastionVariantName(int32 Type,int32 Subclass)
{
    static const char* Names[18][2] = {
        {"LINE BREAKER","ASH VETERAN"},
        {"SPEAR GUARD","HOOK SOLDIER"},
        {"IRON BULWARK","OATH SHIELD"},
        {"DEADEYE","VOLLEY CROSSBOWMAN"},
        {"WIDE CLEAVER","WHIRLWIND HALBERDIER"},
        {"FRENZIED","RAVAGING"},
        {"HUNTER HOUND","BLOOD HOUND"},
        {"DIVING GARGOYLE","FLANKING GARGOYLE"},
        {"EARTHSHAKER","SKULL CRUSHER"},
        {"SHADOW CRAWLER","VENOM CRAWLER"},
        {"CURSED KNIGHT","PIERCING KNIGHT"},
        {"SHADOW HUNTER","VOLLEY HUNTER"},
        {"SIGIL MAGE","RIFT MAGE"},
        {"HEX CALLER","BONE CALLER"},
        {"BRUTAL PUNISHER","WHIRLWIND PUNISHER"},
        {"BLOOD HEALER","FIRE PRIEST"},
        {"SEAL WARDEN","RUNE SLINGER"},
        {"BLADE RAVAGER","FLURRY RAVAGER"}
    };
    if(Type>=34 && Type<=51)
        return Names[Type-34][Subclass!=0?1:0];
    switch(Type){
        case 52:return "EXECUTIONER";
        case 53:return "GUARD CAPTAIN";
        case 54:return "SIEGE GOLEM";
        case 55:return "INQUISITOR";
        case 56:return "THE COMMANDANT";
        default:return "UNKNOWN ENEMY";
    }
}
int32 FAVContent::ShopPrice(int32 Slot, int32 Floor)
{
    const int32 Tier = Floor > 1 ? Floor - 1 : 0;
    switch (Slot)
    {
    case 0: return 16 + 4 * Tier; // 45 HP
    case 1: return 30 + 6 * Tier; // +5 damage
    case 2: return 22 + 4 * Tier; // 1 potion
    default: return 0;
    }
}
