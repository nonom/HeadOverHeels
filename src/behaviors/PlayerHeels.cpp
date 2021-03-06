
#include "PlayerHeels.hpp"
#include "Item.hpp"
#include "DescriptionOfItem.hpp"
#include "PlayerItem.hpp"
#include "Mediator.hpp"
#include "InputManager.hpp"
#include "SoundManager.hpp"


namespace iso
{

PlayerHeels::PlayerHeels( const ItemPtr & item, const std::string& behavior ) :
        UserControlled( item, behavior )
{
        jumpPhases = 20;
        highJumpPhases = 21;

        // salto normal
        for ( unsigned int i = 0; i < jumpPhases; i++ )
        {
                std::pair< int /* xy */, int /* z */ > jumpPhase( i == 9 || i == 19 ? 2 : 1 , ( i < jumpPhases / 2 ) ? 3 : -3 );
                jumpVector.push_back( jumpPhase );
        }

        // salto largo
        for ( unsigned int i = 0; i < highJumpPhases; i++ )
        {
                std::pair< int /* xy */, int /* z */ > jumpPhase( 2 , ( i < 17 ) ? 3 : -3 );
                highJumpVector.push_back( jumpPhase );
        }

        // fotogramas de caída
        fallFrames[ "north" ] = 8;
        fallFrames[ "south" ] = 0;
        fallFrames[ "east" ] = 12;
        fallFrames[ "west" ] = 4;

        // activate chronometers
        speedTimer->go ();
        fallTimer->go ();
}

PlayerHeels::~PlayerHeels( )
{
}

bool PlayerHeels::update()
{
        PlayerItem& playerItem = dynamic_cast< PlayerItem& >( * this->item );

        if ( playerItem.hasShield() )
        {
                playerItem.decreaseShield();
        }

        switch ( activity )
        {
                case Activity::Wait:
                        wait( playerItem );
                        break;

                case Activity::AutoMoveNorth:
                case Activity::AutoMoveSouth:
                case Activity::AutoMoveEast:
                case Activity::AutoMoveWest:
                        autoMove( playerItem );
                        break;

                case Activity::MoveNorth:
                case Activity::MoveSouth:
                case Activity::MoveEast:
                case Activity::MoveWest:
                        move( playerItem );
                        break;

                case Activity::DisplaceNorth:
                case Activity::DisplaceSouth:
                case Activity::DisplaceEast:
                case Activity::DisplaceWest:
                case Activity::DisplaceNortheast:
                case Activity::DisplaceSoutheast:
                case Activity::DisplaceSouthwest:
                case Activity::DisplaceNorthwest:
                case Activity::ForceDisplaceNorth:
                case Activity::ForceDisplaceSouth:
                case Activity::ForceDisplaceEast:
                case Activity::ForceDisplaceWest:
                        displace( playerItem );
                        break;

                case Activity::CancelDisplaceNorth:
                case Activity::CancelDisplaceSouth:
                case Activity::CancelDisplaceEast:
                case Activity::CancelDisplaceWest:
                        cancelDisplace( playerItem );
                        break;

                case Activity::Fall:
                        fall( playerItem );
                        break;

                case Activity::Jump:
                case Activity::RegularJump:
                case Activity::HighJump:
                        jump( playerItem );
                        break;

                case Activity::BeginWayOutTeletransport:
                case Activity::WayOutTeletransport:
                        wayOutTeletransport( playerItem );
                        break;

                case Activity::BeginWayInTeletransport:
                case Activity::WayInTeletransport:
                        wayInTeletransport( playerItem );
                        break;

                case Activity::MeetMortalItem:
                case Activity::Vanish:
                        collideWithMortalItem( playerItem );
                        break;

                case Activity::TakeItem:
                case Activity::TakeAndJump:
                        takeItem( playerItem );
                        break;

                case Activity::ItemTaken:
                        playerItem.addToZ( - LayerHeight );
                        activity = Activity::Wait;
                        break;

                case Activity::DropItem:
                case Activity::DropAndJump:
                        dropItem( playerItem );
                        break;

                default:
                        ;
        }

        // play sound for current activity
        SoundManager::getInstance().play( playerItem.getOriginalLabel(), activity );

        return false;
}

void PlayerHeels::behave ()
{
        PlayerItem& playerItem = dynamic_cast< PlayerItem& >( * this->item );
        InputManager& input = InputManager::getInstance();

        // if it’s not a move by inertia or some other exotic activity
        if ( activity != Activity::AutoMoveNorth && activity != Activity::AutoMoveSouth &&
                activity != Activity::AutoMoveEast && activity != Activity::AutoMoveWest &&
                activity != Activity::BeginWayOutTeletransport && activity != Activity::WayOutTeletransport &&
                activity != Activity::BeginWayInTeletransport && activity != Activity::WayInTeletransport &&
                activity != Activity::MeetMortalItem && activity != Activity::Vanish )
        {
                // when waiting or blinking
                if ( activity == Activity::Wait || activity == Activity::Blink )
                {
                        if ( input.takeTyped() )
                        {
                                activity = ( playerItem.getDescriptionOfTakenItem() == nilPointer ? Activity::TakeItem : Activity::DropItem );
                                input.releaseKeyFor( "take" );
                        }
                        else if ( input.takeAndJumpTyped() )
                        {
                                activity = ( playerItem.getDescriptionOfTakenItem() == nilPointer ? Activity::TakeAndJump : Activity::DropAndJump );
                                input.releaseKeyFor( "take&jump" );
                        }
                        else if ( input.movenorthTyped() )
                        {
                                activity = Activity::MoveNorth;
                        }
                        else if ( input.movesouthTyped() )
                        {
                                activity = Activity::MoveSouth;
                        }
                        else if ( input.moveeastTyped() )
                        {
                                activity = Activity::MoveEast;
                        }
                        else if ( input.movewestTyped() )
                        {
                                activity = Activity::MoveWest;
                        }
                        else if ( input.jumpTyped() )
                        {
                                // look for item below
                                playerItem.canAdvanceTo( 0, 0, -1 );
                                // key to teleport is the same as for jump
                                activity =
                                        playerItem.getMediator()->collisionWithByBehavior( "behavior of teletransport" ) != nilPointer ?
                                                Activity::BeginWayOutTeletransport : Activity::Jump ;
                        }
                }
                // already moving
                else if ( activity == Activity::MoveNorth || activity == Activity::MoveSouth ||
                        activity == Activity::MoveEast || activity == Activity::MoveWest )
                {
                        if( input.jumpTyped() )
                        {
                                // teleport when teletransport is below
                                playerItem.canAdvanceTo( 0, 0, -1 );
                                activity =
                                        playerItem.getMediator()->collisionWithByBehavior( "behavior of teletransport" ) != nilPointer ?
                                                Activity::BeginWayOutTeletransport : Activity::Jump ;
                        }
                        else if ( input.takeTyped() )
                        {
                                activity = ( playerItem.getDescriptionOfTakenItem() == nilPointer ? Activity::TakeItem : Activity::DropItem );
                                input.releaseKeyFor( "take" );
                        }
                        else if ( input.takeAndJumpTyped() )
                        {
                                activity = ( playerItem.getDescriptionOfTakenItem() == nilPointer ? Activity::TakeAndJump : Activity::DropAndJump );
                                input.releaseKeyFor( "take&jump" );
                        }
                        else if ( input.movenorthTyped() )
                        {
                                activity = Activity::MoveNorth;
                        }
                        else if ( input.movesouthTyped() )
                        {
                                activity = Activity::MoveSouth;
                        }
                        else if ( input.moveeastTyped() )
                        {
                                activity = Activity::MoveEast;
                        }
                        else if ( input.movewestTyped() )
                        {
                                activity = Activity::MoveWest;
                        }
                        else if ( ! input.anyMoveTyped() )
                        {
                                SoundManager::getInstance().stop( playerItem.getLabel(), activity );
                                activity = Activity::Wait;
                        }
                }
                // if you are being displaced
                else if ( activity == Activity::DisplaceNorth || activity == Activity::DisplaceSouth ||
                        activity == Activity::DisplaceEast || activity == Activity::DisplaceWest )
                {
                        if ( input.jumpTyped() )
                        {
                                activity = Activity::Jump;
                        }
                        else if ( input.takeTyped() )
                        {
                                activity = ( playerItem.getDescriptionOfTakenItem() == nilPointer ? Activity::TakeItem : Activity::DropItem );
                                input.releaseKeyFor( "take" );
                        }
                        else if ( input.takeAndJumpTyped() )
                        {
                                activity = ( playerItem.getDescriptionOfTakenItem() == nilPointer ? Activity::TakeAndJump : Activity::DropAndJump );
                                input.releaseKeyFor( "take&jump" );
                        }
                        else if ( input.movenorthTyped() )
                        {
                                activity = Activity::MoveNorth;
                        }
                        else if ( input.movesouthTyped() )
                        {
                                activity = Activity::MoveSouth;
                        }
                        else if ( input.moveeastTyped() )
                        {
                                activity = Activity::MoveEast;
                        }
                        else if ( input.movewestTyped() )
                        {
                                activity = Activity::MoveWest;
                        }
                }
                // if you are being forcibly displaced
                else if ( activity == Activity::ForceDisplaceNorth || activity == Activity::ForceDisplaceSouth ||
                        activity == Activity::ForceDisplaceEast || activity == Activity::ForceDisplaceWest )
                {
                        if ( input.jumpTyped() )
                        {
                                activity = Activity::Jump;
                        }
                        // user moves while displacing
                        // cancel displace when moving in direction opposite to displacement
                        else if ( input.movenorthTyped() )
                        {
                                activity = ( activity == Activity::ForceDisplaceSouth ? Activity::CancelDisplaceSouth : Activity::MoveNorth );
                        }
                        else if ( input.movesouthTyped() )
                        {
                                activity = ( activity == Activity::ForceDisplaceNorth ? Activity::CancelDisplaceNorth : Activity::MoveSouth );
                        }
                        else if ( input.moveeastTyped() )
                        {
                                activity = ( activity == Activity::ForceDisplaceWest ? Activity::CancelDisplaceWest : Activity::MoveEast );
                        }
                        else if ( input.movewestTyped() )
                        {
                                activity = ( activity == Activity::ForceDisplaceEast ? Activity::CancelDisplaceEast : Activity::MoveWest );
                        }
                }
                else if ( activity == Activity::Jump || activity == Activity::RegularJump || activity == Activity::HighJump )
                {

                }
                else if ( activity == Activity::Fall )
                {
                        // pick or drop an item when falling
                        if ( input.takeTyped() )
                        {
                                activity = ( playerItem.getDescriptionOfTakenItem() == nilPointer ? Activity::TakeItem : Activity::DropItem );
                                input.releaseKeyFor( "take" );
                        }
                }
        }
}

}
