/***************************************************************************
 *   Free Heroes of Might and Magic II: https://github.com/ihhub/fheroes2  *
 *   Copyright (C) 2020 - 2022                                             *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#ifndef H2AI_NORMAL_H
#define H2AI_NORMAL_H

#include "ai.h"
#include "world_pathfinding.h"

#include <set>

struct KingdomCastles;

namespace Battle
{
    class Units;
}

namespace AI
{
    struct RegionStats
    {
        bool evaluated = false;
        double highestThreat = -1;
        double averageMonster = -1;
        int friendlyHeroes = 0;
        int friendlyCastles = 0;
        int enemyCastles = 0;
        int monsterCount = 0;
        int fogCount = 0;
        int safetyFactor = 0;
        int spellLevel = 2;
        std::vector<IndexObject> validObjects;
    };

    struct AICastle
    {
        Castle * castle = nullptr;
        bool underThreat = false;
        int safetyFactor = 0;
        int buildingValue = 0;
        AICastle( Castle * inCastle, bool inThreat, int inSafety, int inValue )
            : castle( inCastle )
            , underThreat( inThreat )
            , safetyFactor( inSafety )
            , buildingValue( inValue )
        {
            assert( castle != nullptr );
        }
    };

    struct HeroToMove
    {
        Heroes * hero = nullptr;
        int patrolCenter = -1;
        uint32_t patrolDistance = 0;
    };

    struct BattleTargetPair
    {
        int cell = -1;
        const Battle::Unit * unit = nullptr;
    };

    struct SpellSelection
    {
        int spellID = -1;
        int32_t cell = -1;
        double value = 0.0;
    };

    struct SpellcastOutcome
    {
        int32_t cell = -1;
        double value = 0.0;

        void updateOutcome( const double potentialValue, const int32_t targetCell, const bool isMassEffect = false )
        {
            if ( isMassEffect ) {
                value += potentialValue;
            }
            else if ( potentialValue > value ) {
                value = potentialValue;
                cell = targetCell;
            }
        }
    };

    class BattlePlanner
    {
    public:
        Battle::Actions planUnitTurn( Battle::Arena & arena, const Battle::Unit & currentUnit );
        void analyzeBattleState( const Battle::Arena & arena, const Battle::Unit & currentUnit );

        // decision-making helpers
        bool isUnitFaster( const Battle::Unit & currentUnit, const Battle::Unit & target ) const;
        bool isHeroWorthSaving( const Heroes & hero ) const;
        bool isCommanderCanSpellcast( const Battle::Arena & arena, const HeroBase * commander ) const;
        bool checkRetreatCondition( const Heroes & hero ) const;

    private:
        // to be exposed later once every BattlePlanner will be re-initialized at combat start
        Battle::Actions berserkTurn( const Battle::Arena & arena, const Battle::Unit & currentUnit ) const;
        Battle::Actions archerDecision( const Battle::Arena & arena, const Battle::Unit & currentUnit ) const;
        BattleTargetPair meleeUnitOffense( const Battle::Arena & arena, const Battle::Unit & currentUnit ) const;
        BattleTargetPair meleeUnitDefense( const Battle::Arena & arena, const Battle::Unit & currentUnit ) const;
        SpellSelection selectBestSpell( Battle::Arena & arena, const Battle::Unit & currentUnit, bool retreating ) const;
        SpellcastOutcome spellDamageValue( const Spell & spell, Battle::Arena & arena, const Battle::Unit & currentUnit, const Battle::Units & friendly,
                                           const Battle::Units & enemies, bool retreating ) const;
        SpellcastOutcome spellDispellValue( const Spell & spell, const Battle::Units & friendly, const Battle::Units & enemies ) const;
        SpellcastOutcome spellResurrectValue( const Spell & spell, const Battle::Arena & arena ) const;
        SpellcastOutcome spellSummonValue( const Spell & spell, const Battle::Arena & arena, const int heroColor ) const;
        SpellcastOutcome spellEffectValue( const Spell & spell, const Battle::Units & targets ) const;
        double spellEffectValue( const Spell & spell, const Battle::Unit & target, bool targetIsLast, bool forDispell ) const;
        double getSpellDisruptingRayRatio( const Battle::Unit & target ) const;
        double getSpellSlowRatio( const Battle::Unit & target ) const;
        double getSpellHasteRatio( const Battle::Unit & target ) const;
        int32_t spellDurationMultiplier( const Battle::Unit & target ) const;
        static double commanderMaximumSpellDamageValue( const HeroBase & commander );

        // turn variables that wouldn't persist
        const HeroBase * _commander = nullptr;
        int _myColor = Color::NONE;
        double _myArmyStrength = 0;
        double _enemyArmyStrength = 0;
        double _myShooterStr = 0;
        double _enemyShooterStr = 0;
        double _myArmyAverageSpeed = 0;
        double _enemyAverageSpeed = 0;
        double _enemySpellStrength = 0;
        int _highestDamageExpected = 0;
        bool _attackingCastle = false;
        bool _defendingCastle = false;
        bool _considerRetreat = false;
        bool _defensiveTactics = false;

        const Rand::DeterministicRandomGenerator * _randomGenerator = nullptr;
    };

    class Normal : public Base
    {
    public:
        Normal();
        void KingdomTurn( Kingdom & kingdom ) override;
        void CastleTurn( Castle & castle, bool defensive ) override;
        void BattleTurn( Battle::Arena & arena, const Battle::Unit & currentUnit, Battle::Actions & actions ) override;
        bool HeroesTurn( VecHeroes & heroes ) override;

        void revealFog( const Maps::Tiles & tile ) override;

        void HeroesPreBattle( HeroBase & hero, bool isAttacking ) override;
        void HeroesActionComplete( Heroes & hero ) override;

        bool recruitHero( Castle & castle, bool buyArmy, bool underThreat );
        void evaluateRegionSafety();
        std::set<int> findCastlesInDanger( const KingdomCastles & castles, const std::vector<std::pair<int, const Army *>> & enemyArmies, int myColor );
        std::vector<AICastle> getSortedCastleList( const KingdomCastles & castles, const std::set<int> & castlesInDanger );

        double getObjectValue( const Heroes & hero, const int index, const double valueToIgnore, const uint32_t distanceToObject ) const;
        int getPriorityTarget( const HeroToMove & heroInfo, double & maxPriority );
        void resetPathfinder() override;

    private:
        // following data won't be saved/serialized
        double _combinedHeroStrength = 0;
        std::vector<IndexObject> _mapObjects;
        std::vector<RegionStats> _regions;
        AIWorldPathfinder _pathfinder;
        BattlePlanner _battlePlanner;

        double getHunterObjectValue( const Heroes & hero, const int index, const double valueToIgnore, const uint32_t distanceToObject ) const;

        double getFighterObjectValue( const Heroes & hero, const int index, const double valueToIgnore, const uint32_t distanceToObject ) const;
    };
}

#endif
