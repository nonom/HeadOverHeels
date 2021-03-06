
#include "ShowAuthors.hpp"
#include "GuiManager.hpp"
#include "LanguageText.hpp"
#include "LanguageManager.hpp"
#include "SoundManager.hpp"

#include "Font.hpp"
#include "Screen.hpp"
#include "Label.hpp"
#include "PictureWidget.hpp"
#include "TextField.hpp"
#include "CreateMainMenu.hpp"

using gui::ShowAuthors;
using gui::Label;
using gui::PictureWidget;
using gui::TextField;
using iso::SoundManager;


ShowAuthors::ShowAuthors( )
        : Action( )
        , linesOfCredits( nilPointer )
        , initialY( 0 )
        , loadingScreen( )
{
}

ShowAuthors::~ShowAuthors( )
{
        delete linesOfCredits ;
}

void ShowAuthors::doAction ()
{
        SoundManager::getInstance().playOgg( "music/CreditsTheme.ogg", /* loop */ true );

        Screen& screen = * GuiManager::getInstance().findOrCreateScreenForAction( this );
        if ( screen.countWidgets() == 0 )
        {
                LanguageManager* languageManager = GuiManager::getInstance().getLanguageManager();
                LanguageText* langString = languageManager->findLanguageStringForAlias( "credits-text" );

                this->initialY = screen.getImageOfScreen().getHeight() ;
                this->linesOfCredits = new TextField( iso::ScreenWidth(), "center" );
                this->linesOfCredits->moveTo( 0, initialY );

                size_t howManyLines = langString->getLinesCount() ;
                std::cout << "credits-text has " << howManyLines << " lines" << std::endl ;

                for ( size_t i = 0; i < howManyLines; i++ )
                {
                        LanguageLine* line = langString->getLine( i );
                        linesOfCredits->addLine( line->text, line->font, line->color );
                }

                screen.addWidget( linesOfCredits );
        }
        else
        {
                // restore initial position
                linesOfCredits->moveTo( linesOfCredits->getX(), initialY );
        }

        screen.setEscapeAction( new CreateMainMenu() );

        GuiManager::getInstance().changeScreen( screen, true );

        const unsigned int widthOfSlide = screen.getImageOfScreen().getWidth() ;
        const unsigned int heightOfSlide = screen.getImageOfScreen().getHeight() ;

        const unsigned int heightOfCredits = ( ( linesOfCredits->getHeightOfField() + 1 ) >> 1 ) << 1;
        const unsigned int verticalSpace = ( heightOfSlide * 3 ) >> 2;
        const int whenToReloop = - ( heightOfCredits + verticalSpace ) ;

        std::cout << "height of credits-text is " << heightOfCredits << std::endl ;

        PictureWidget* widgetForLoadingScreen = nilPointer;

        // move text up

        while ( ! screen.getEscapeAction()->hasBegun() )
        {
                int yNow = linesOfCredits->getY() - 1;

                if ( allegro::isKeyPushed( "Space" ) && allegro::isShiftKeyPushed() )
                {
                        if ( allegro::isAltKeyPushed() )
                                yNow += 2 ;
                        else
                                yNow ++ ;
                }

                if ( allegro::isAltKeyPushed() && allegro::isShiftKeyPushed() && allegro::isKeyPushed( "Left" ) )
                {
                        linesOfCredits->moveTo( linesOfCredits->getX () - 1, linesOfCredits->getY () );
                }
                if ( allegro::isAltKeyPushed() && allegro::isShiftKeyPushed() && allegro::isKeyPushed( "Right" ) )
                {
                        linesOfCredits->moveTo( linesOfCredits->getX () + 1, linesOfCredits->getY () );
                }
                if ( allegro::isAltKeyPushed() && allegro::isShiftKeyPushed() && allegro::isKeyPushed( "Pad 0" ) )
                {
                        linesOfCredits->moveTo( 0, linesOfCredits->getY () );
                }

                if ( allegro::isAltKeyPushed() && allegro::isShiftKeyPushed() && allegro::isKeyPushed( "l" ) )
                {
                        linesOfCredits->setAlignment( "left" );
                }
                if ( allegro::isAltKeyPushed() && allegro::isShiftKeyPushed() && allegro::isKeyPushed( "c" ) )
                {
                        linesOfCredits->setAlignment( "center" );
                }
                if ( allegro::isAltKeyPushed() && allegro::isShiftKeyPushed() && allegro::isKeyPushed( "r" ) )
                {
                        linesOfCredits->setAlignment( "right" );
                }

                if ( yNow <= whenToReloop )
                {
                        // loop it
                        yNow = initialY ;
                }

                linesOfCredits->moveTo( linesOfCredits->getX(), yNow );

                if ( yNow == static_cast< int >( heightOfSlide ) - static_cast< int >( heightOfCredits ) && widgetForLoadingScreen == nilPointer )
                {
                        if ( loadingScreen == nilPointer )
                        {
                                autouniqueptr< allegro::Pict > png( allegro::Pict::fromPNGFile (
                                        iso::pathToFile( iso::sharePath(), "loading-screen.png" )
                                ) );
                                loadingScreen = PicturePtr( new Picture( * png ) );
                                loadingScreen->setName( "image of loading screen from original speccy version" );
                        }

                        if ( loadingScreen->getAllegroPict().isNotNil() )
                        {
                                widgetForLoadingScreen = new PictureWidget (
                                                ( widthOfSlide - loadingScreen->getWidth() ) >> 1, heightOfSlide,
                                                loadingScreen,
                                                "loading screen from original speccy version"
                                ) ;
                                screen.addWidget( widgetForLoadingScreen );
                        }
                }
                else if ( yNow < static_cast< int >( heightOfSlide ) - static_cast< int >( heightOfCredits ) && widgetForLoadingScreen != nilPointer )
                {
                        widgetForLoadingScreen->moveTo( widgetForLoadingScreen->getX(), yNow + heightOfCredits );
                }
                else if ( widgetForLoadingScreen != nilPointer )
                {
                        screen.removeWidget( widgetForLoadingScreen );
                        widgetForLoadingScreen = nilPointer;
                }

                GuiManager::getInstance().redraw ();

                if ( allegro::isAltKeyPushed() && allegro::isShiftKeyPushed() && allegro::isKeyPushed( "f" ) )
                {
                        gui::GuiManager::getInstance().toggleFullScreenVideo ();
                }

                if ( ! allegro::isKeyPushed( "Space" ) )
                {
                        milliSleep( 20 );
                }

                if ( allegro::isKeyPushed( "Escape" ) )
                {
                        allegro::emptyKeyboardBuffer();
                        screen.handleKey( "Escape" );
                }
        }

        if ( widgetForLoadingScreen != nilPointer )
        {
                screen.removeWidget( widgetForLoadingScreen );
        }
}
