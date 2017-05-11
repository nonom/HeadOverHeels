// The free and open source remake of Head over Heels
//
// Copyright © 2017 Douglas Mencken dougmencken@gmail.com
// Copyright © 2008 Jorge Rodríguez Santos
// Original game copyright © 1987 Ocean Software Ltd.
//
// This program is free software
// You may redistribute it and~or modify it under the terms of the GNU General Public License
// either version 3 of the License or at your option any later version

#ifndef GuiManager_hpp_
#define GuiManager_hpp_

#include <string>
#include <list>
#include <map>
#include <cassert>
#include <allegro.h>
#include <loadpng.h>
#include "Gui.hpp"

namespace gui
{

class Screen;
class Action;
class LanguageManager;
class ConfigurationManager;

/**
 * El gestor de la interfaz usuario almacena las imágenes empleadas por la interfaz, ordena
 * la creación de los tipos de letra usados e inicia el juego mostrando el menú principal
 */
class GuiManager
{

private:

       /**
        * Constructor
        * Carga y almacena el conjunto de imágenes empleadas por la interfaz de
        * usuario, así como manda crear todos las fuentes tipográficas utilizadas
        */
        GuiManager( ) ;

public:

        ~GuiManager( ) ;

       /**
        * Único objeto de esta clase para toda la aplicación
        * @return Un puntero al objeto único
        */
        static GuiManager* getInstance () ;

       /**
        * Begin with the menu for this game
        */
        void begin () ;

       /**
        * Change the current screen
        */
        void changeScreen ( Screen* newScreen ) ;

       /*
        * Search in list of screens for the one associated with this action
        * When there's no such screen found, create a new one with a given picture
        */
        Screen * findOrCreateScreenForAction ( Action * action, BITMAP * picture ) ;

        void freeScreens () ;

        void refreshScreens () ;

       /**
        * Actualiza el contenido de la pantalla
        */
        void refresh () ;

        std::string getPathToPicturesOfGui () ;

       /**
        * Establece el idioma de la interfaz de usuario
        * @param language Código ISO de la lengua
        */
        void assignLanguage ( const std::string& language ) ;

       /**
        * Suspende la presentación de la interfaz de usuario. Conduce al cierre de la aplicación
        */
        void suspend () {  this->active = false ;  }

        bool isAtFullScreen () ;

       /**
        * Use it to toggle video at full screen & video in window
        */
        void toggleFullScreenVideo () ;

protected:

       /**
        * Inicializa Allegro, establece el modo de vídeo e instala el controlador del teclado
        */
        void allegroSetup () ;

private:

       /**
        * Único objeto de esta clase para toda la aplicación
        */
        static GuiManager * instance ;

       /**
        * Screen to display by interface
        */
        Screen * screen ;

        std::map < std::string, Screen * > listOfScreens;

       /**
        * Imagen donde se dibujará la interfaz gráfica
        */
        BITMAP * picture ;

       /**
        * El gestor de la configuración del juego
        */
        ConfigurationManager * configurationManager ;

       /**
        * El gestor de idioma
        */
        LanguageManager * languageManager ;

       /**
        * Indica que la presentación de la interfaz y la gestión del teclado está activa
        */
        bool active ;

       /**
        * Draw graphics at full screen when true or in window when false
        */
        bool atFullScreen ;

public: // Operaciones de consulta y actualización

       /**
        * Devuelve el gestor de la configuración del juego
        * @return Un puntero al gestor ó 0 si no está creado
        */
        ConfigurationManager* getConfigurationManager () const ;

       /**
        * Devuelve el gestor de idioma
        * @return Un puntero al gestor ó 0 si no está creado
        */
        LanguageManager* getLanguageManager () const ;

};

}

#endif
