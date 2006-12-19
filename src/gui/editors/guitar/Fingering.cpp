#include "Fingering.h"
#include "FingeringBox.h"
#include <iostream>
#include <sstream>
#include "NoteSymbols.h"
#include "base/Exception.h"
#include "misc/Debug.h"

namespace Rosegarden
{

namespace Guitar
{

const std::string Fingering::EventType = "fingering";
const short Fingering::EventSubOrdering = -60;

/*---------------------------------------------------------------
	Fingering
---------------------------------------------------------------*/
Fingering::Fingering ()
        : m_guitar ( new GuitarNeck() ),
        m_startFret ( 1 ),
        m_frets_displayed( FingeringBox::MAX_FRET_DISPLAYED ),
        m_transientStringNb( 0 ),
        m_transientFretNb( 0 )
{}

Fingering::Fingering( GuitarNeck* gPtr, unsigned int nbFretsDisplayed )
        : m_guitar ( new GuitarNeck( *gPtr ) ),
        m_startFret ( 1 ),
        m_frets_displayed ( nbFretsDisplayed )
{}

Fingering::Fingering ( Fingering const& rhs )
        : m_guitar( new GuitarNeck( *rhs.m_guitar ) ),
        m_startFret( rhs.m_startFret ),
        m_frets_displayed ( rhs.m_frets_displayed )
{
    for ( BarreMap::const_iterator pos = rhs.m_barreFretMap.begin();
            pos != rhs.m_barreFretMap.end();
            ++pos ) {
        Barre* b_ptr = ( *pos ).second;
        this->addBarre ( new Barre( *b_ptr ) );
    }

    for ( NoteMap::const_iterator pos = rhs.m_notes.begin();
            pos != rhs.m_notes.end();
            ++pos ) {
        Note* n_ptr = ( *pos ).second;
        this->addNote ( new Note( *n_ptr ) );
    }


}

Fingering::~Fingering ()
{
    delete m_guitar;
    for ( NoteMap::iterator pos = m_notes.begin();
            pos != m_notes.end();
            ++pos ) {
        delete pos->second;
    }

    for ( BarreStringMap::iterator pos = m_barreStringMap.begin();
            pos != m_barreStringMap.end();
            ++pos ) {
        delete pos->second;
    }

    /*
        for ( BarreMap::iterator pos = m_barreFretMap.begin();
                pos != m_barreFretMap.end();
                ++pos )
        {
            delete pos->second;
        }
    */
}

Fingering::Fingering ( Event const& e_ref )
{

    // Create Barre List
    std::stringstream output;
    e_ref.dump ( output );

    std::cout << "Fingering::Fingering (Event) - dump Event" << std::endl
    << output.str() << std::endl;


    // Restore GuitarNeck pointer
    unsigned int max_strings;
    unsigned int max_frets;

    e_ref.get<UInt>( PropertyName( "GUITAR_MAXFRETS" ),
                     max_frets );
    e_ref.get<UInt>( PropertyName( "GUITAR_MAXSTRINGS" ),
                     max_strings );
    m_guitar = new GuitarNeck( max_strings, max_frets );

    // Resotre start fret
    unsigned int startFret;
    e_ref.get<UInt>( PropertyName( "STARTFRET" ),
                     startFret );
    m_startFret = startFret;

    // Restore Note List
    unsigned int note_size = m_notes.size();
    e_ref.get<UInt>( PropertyName( "NOTE_NUM" ),
                     note_size );

    if ( note_size > 0 ) {

        // - For x in NoteList
        for ( unsigned int i = 1;
                i <= note_size;
                ++i ) {
            unsigned int string_num;
            QString name = QString( "NOTE%1.%2" )
                           .arg( i )
                           .arg( "STRING" );
            e_ref.get<UInt>( PropertyName( name.ascii() ),
                             string_num );

            unsigned int fret_num;
            name = QString( "NOTE%1.%2" )
                   .arg( i )
                   .arg( "FRET" );
            e_ref.get<UInt>( PropertyName( name.ascii() ),
                             fret_num );

            //   - Add name + value to event
            unsigned int note_act;
            name = QString( "NOTE%1.%2" )
                   .arg( i )
                   .arg( "ACTION" );
            e_ref.get<UInt>( PropertyName( name.ascii() ),
                             note_act );

            Note* n_ptr = new Note ( string_num, fret_num );
            if ( ! this->addNote ( n_ptr ) ) {
                delete n_ptr;
            }
            m_guitar->setStringStatus( string_num,
                                       GuitarString::actionValue( note_act ) );
        }
    }

    // - Add number of Barres to event
    unsigned int barre_size;
    e_ref.get<UInt>( PropertyName( "BARRE_NUM" ), barre_size );

    if ( barre_size > 0 ) {
        // - For x in BarreList
        for ( unsigned int i = 1;
                i <= barre_size;
                ++i ) {
            unsigned int fret_num;
            QString name = QString( "BARRE%1.%2" )
                           .arg( i )
                           .arg( "FRET" );
            e_ref.get<UInt>( PropertyName( name.ascii() ),
                             fret_num );

            unsigned int start_string;
            name = QString( "BARRE%1.%2" )
                   .arg( i )
                   .arg( "START" );
            e_ref.get<UInt>( PropertyName( name.ascii() ),
                             start_string );

            unsigned int end_string;
            name = QString( "BARRE%1.%2" )
                   .arg( i )
                   .arg( "END" );
            e_ref.get<UInt>( PropertyName( name.ascii() ),
                             end_string );

            Barre* b_ptr = new Barre ( fret_num, start_string, end_string );
            this->addBarre( b_ptr );
        }
    }
}

void Fingering::setFirstFret ( unsigned int fret )
{
    int fret_change = fret - m_startFret;
    /*
    	std::cout << "Fingering::setFirstFret - fret: " << fret << std::endl;
    	std::cout << "Fingering::setFirstFret - start fret: " << m_startFret << std::endl;
    	std::cout << "Fingering::setFirstFret - fret change: " << fret_change << std::endl;
    */

    for ( NoteMap::iterator pos = m_notes.begin();
            pos != m_notes.end();
            ++pos ) {
        NoteMapPair nPair = ( *pos );
        Note* nPtr = nPair.second;
        nPtr->setFirstFret( fret_change );
    }

    // This is incorrect. We need to call setFirstFret on all Barres but
    // we must also remove and reinsert adjusted bar to BarreMap
    for ( BarreMap::iterator pos = m_barreFretMap.begin();
            pos != m_barreFretMap.end();
            ++pos ) {
        // Change the fret number in the Barre
        BarreMapPair bPair = ( *pos );
        Barre* bPtr = bPair.second;
        bPtr->setFirstFret( fret_change );

        // Remove old pair
        m_barreFretMap.erase( pos );

        // Add new pair
        this->addBarre ( bPtr );
    }

    m_startFret = fret;
}

Note*
Fingering::getNote ( unsigned int string_num )
{
    if ( ( string_num > 0 ) && ( string_num <= m_guitar->getNumberOfStrings() ) ) {
        return m_notes[ string_num ];
    } else {
        std::stringstream error;
        error << "Invalid string number given (" << string_num
        << ") expected to see a value within the range of (1 - "
        << m_guitar->getNumberOfStrings() << ")\n";

        throw Exception ( error.str() );
    }
}

Barre*
Fingering::getBarre ( unsigned int fret_num )
{
    if ( ( fret_num > 0 ) && ( fret_num <= m_guitar->getNumberOfFrets() ) ) {
        return m_barreFretMap[ fret_num ];
    } else {
        QString error = QString ( "Invalid fret number given (%1) expected to see a value within the range of (1 - %2)\n" )
                        .arg ( fret_num )
                        .arg ( m_guitar->getNumberOfFrets() );

        throw Exception ( error );
    }
}

void Fingering::drawContents ( QPainter* p ) const
{
    // CHANGE: New system using the GuitarStrings to denote status
    // For all strings on guitar
    //   check state of string
    //     If pressed display note
    //     Else display muted or open symbol
    // For all bars
    //   display bar
    // Horizontal separator line
    NoteSymbols ns(m_guitar->getNumberOfStrings(), m_frets_displayed);

    ns.drawFretNumber ( p, m_startFret );
    ns.drawFrets ( p );
    ns.drawStrings ( p );

    for ( GuitarNeck::GuitarStringMap::const_iterator pos = m_guitar->begin();
            pos != m_guitar->end();
            ++pos ) {
        GuitarString* g_ptr = ( *pos ).second;
        switch ( g_ptr->m_state ) {
        case GuitarString::PRESSED: {
                NoteMap::const_iterator notePos = m_notes.find( ( *pos ).first );
                if ( notePos != m_notes.end() ) {
                    NoteMapPair nPair = ( *notePos );
                    Note* nPtr = nPair.second;
                    nPtr->drawContents ( p,
                                         m_startFret,
                                         m_guitar->getNumberOfStrings(),
                                         m_frets_displayed );
                }
                break;
            }
        case GuitarString::OPEN: {
                //std::cout << "Fingering::drawContents - drawing Open symbol" << std::endl;
                ns.drawOpenSymbol( p,
                                   m_guitar->getNumberOfStrings() - ( *pos ).first );

                break;
            }
        case GuitarString::MUTED: {
                //std::cout << "Fingering::drawContents - drawing Mute symbol" << std::endl;
                ns.drawMuteSymbol( p,
                                   m_guitar->getNumberOfStrings() - ( *pos ).first );

                break;
            }
        }
    }

    for ( BarreMap::const_iterator pos = m_barreFretMap.begin();
            pos != m_barreFretMap.end();
            ++pos ) {
        BarreMapPair bPair = ( *pos );
        Barre* bPtr = bPair.second;
        bPtr->drawContents ( p,
                             m_startFret,
                             m_guitar->getNumberOfStrings(),
                             m_frets_displayed );
    }
    
    if (m_transientFretNb > 0 && m_transientFretNb <= m_frets_displayed &&
        m_transientStringNb > 0 && m_transientStringNb <= m_guitar->getNumberOfStrings()) {
        ns.drawNoteSymbol(p, m_guitar->getNumberOfStrings() - m_transientStringNb, m_transientFretNb - m_startFret,
                         true);
    }
    
}

QRect
Fingering::updateTransientPos(QSize fretboardSize, unsigned int stringNb, unsigned int fretNb)
{
    unsigned int previousTransientStringNb = m_transientStringNb;
    unsigned int previousTransientFretNb   = m_transientFretNb;
    
    NoteSymbols ns(m_guitar->getNumberOfStrings(), m_frets_displayed);
    QRect r1 = ns.getTransientNoteSymbolRect(fretboardSize,
                                             m_guitar->getNumberOfStrings() - previousTransientStringNb,
                                             previousTransientFretNb - m_startFret);
    m_transientStringNb = stringNb;
    m_transientFretNb   = fretNb;
    QRect r2 = ns.getTransientNoteSymbolRect(fretboardSize,
                                             m_guitar->getNumberOfStrings() - m_transientStringNb,
                                             m_transientFretNb - m_startFret);
    
//    RG_DEBUG << "Fingering::updateTransientPos r1 = " << r1 << " - r2 = " << r2 << endl;
     
    return r1 | r2;
}

bool
Fingering::addNote ( Note* notePtr )
{
    bool result = false;

    unsigned int stringPos = notePtr->getStringNumber();
    unsigned int notePos = notePtr->getFret();

    if ( ( stringPos > 0 ) &&
            ( stringPos <= m_guitar->getNumberOfStrings() ) &&
            ( notePos >= 0 ) ) {
        m_notes.insert ( std::make_pair( stringPos, notePtr ) );
        result = true;
    }

    return result;
}

void
Fingering::removeNote ( unsigned int string_num )
{
    if ( ( string_num > 0 ) &&
            ( string_num <= m_guitar->getNumberOfStrings() ) ) {
        NoteMap::iterator pos = m_notes.find ( string_num );
        if ( pos != m_notes.end() ) {
            delete ( *pos ).second;
            m_notes.erase( pos );
        }
    }

    BarreStringMap::const_iterator bListPos = m_barreStringMap.find ( string_num );
    if ( bListPos != m_barreStringMap.end() ) {
        BarreList* bListPtr = ( *bListPos ).second;
        if ( bListPtr->empty() ) {
            m_guitar->setStringStatus( string_num, GuitarString::MUTED );
        }
    }
}

void
Fingering::removeBarre ( unsigned int fret_num )
{
    BarreMap::iterator pos = m_barreFretMap.find ( fret_num );
    Barre* barrePtr = ( *pos ).second;

    if ( pos != m_barreFretMap.end() ) {
        m_barreFretMap.erase ( pos );
    }

    for ( unsigned int i = barrePtr->getStart();
            i >= barrePtr->getEnd();
            --i ) {
        BarreStringMap::iterator bPos = m_barreStringMap.find( i );
        BarreList * bListPtr;

        if ( bPos != m_barreStringMap.end() ) {
            bListPtr = ( *bPos ).second;
            bListPtr->erase( barrePtr );
        }

        if ( ( ! this->hasNote( i ) ) &&
                bListPtr->empty() ) {
            m_guitar->setStringStatus( i, GuitarString::MUTED );
        }
    }
    delete barrePtr;
}

bool Fingering::hasNote ( unsigned int stringPos )
{
    bool result = false;

    if ( ( stringPos > 0 ) && ( stringPos <= m_guitar->getNumberOfStrings() ) ) {
        NoteMap::iterator noteMapPos = m_notes.find( stringPos );

        if ( noteMapPos != m_notes.end() ) {
            result = true;
        }
    }

    return result;
}

void Fingering::addBarre ( Barre* barrePtr )
{
    m_barreFretMap.insert ( std::make_pair( barrePtr->getFret(), barrePtr ) );

    for ( unsigned int i = barrePtr->getStart();
            i >= barrePtr->getEnd();
            --i ) {
        BarreStringMap::const_iterator bPos = m_barreStringMap.find( i );
        if ( bPos == m_barreStringMap.end() ) {
            m_barreStringMap.insert ( std::make_pair( i, new BarreList() ) );
        }

        BarreList* barreListPtr = m_barreStringMap[ i ];
        barreListPtr->push_back ( barrePtr );

        m_guitar->setStringStatus( i, GuitarString::PRESSED );
    }
}

bool
Fingering::hasBarre ( unsigned int fret_num )
{
    bool result = false;
    if ( ( fret_num > 0 ) && ( fret_num <= m_guitar->getNumberOfFrets() ) ) {
        BarreMap::const_iterator pos = m_barreFretMap.find ( fret_num );
        if ( pos != m_barreFretMap.end() ) {
            result = true;
        }
    }
    return result;
}

std::string
Fingering::toString ( void ) const
{
    std::stringstream output;
    output << "  Fingering: " << std::endl;
    output << "  start fret #" << m_startFret << std::endl;
    output << m_guitar->toString();

    for ( NoteMap::const_iterator pos = m_notes.begin();
            pos != m_notes.end();
            ++pos ) {
        NoteMapPair nPair = ( *pos );
        Note* notePtr = nPair.second;
        output << notePtr->toString();
    }

    for ( BarreMap::const_iterator pos = m_barreFretMap.begin();
            pos != m_barreFretMap.end();
            ++pos ) {
        BarreMapPair bPair = ( *pos );
        Barre* barrePtr = bPair.second;
        output << barrePtr->toString();
    }

    return output.str();
}

void Fingering::setStringStatus ( unsigned int stringPos, GuitarString::Action action )
{
    m_guitar->setStringStatus( stringPos, action );
}

GuitarString::Action const&
Fingering::getStringStatus ( unsigned int stringPos ) const
{
    return m_guitar->getStringStatus( stringPos );
}

void Fingering::load ( QDomNode const& node_ref )
{
    QDomNode node = node_ref;

    // Parse Barre or Note sequence
    while ( ( node.nodeType() == QDomNode::ElementNode ) &&
            ( ( node.nodeName() == "Barre" ) ||
              ( node.nodeName() == "Note" ) ) ) {
        QString lcName = node.nodeName();
        if ( lcName.lower() == "barre" ) {
            Barre * barrePtr = new Barre();
            barrePtr->load ( node );
            this->addBarre ( barrePtr );
        } else if ( lcName.lower() == "note" ) {
            Note * notePtr = new Note();
            notePtr->load ( node, m_guitar );
            if ( ! this->addNote ( notePtr ) ) {
                delete notePtr;
            }
        }

        node = node.nextSibling();

        if ( node.isNull() ) {
            break;
        }

        while ( node.nodeType() == QDomNode::CommentNode ) {
            node = node.nextSibling();
        }
    }
}

void Fingering::save ( QDomNode& parent_node )
{
    /*
    	std::cout << "Number of barres: " << m_barreFretMap.size() << std::endl;
    	std::cout << "Number of notes: " << m_notes.size() << std::endl;
    */
    for ( BarreMap::const_iterator pos = m_barreFretMap.begin();
            pos != m_barreFretMap.end();
            ++pos ) {
        BarreMapPair bPair = ( *pos );
        Barre* barrePtr = bPair.second;
        barrePtr->save ( parent_node );
    }

    for ( NoteMap::const_iterator pos = m_notes.begin();
            pos != m_notes.end();
            ++pos ) {
        NoteMapPair nPair = ( *pos );
        Note* notePtr = nPair.second;
        notePtr->save ( parent_node, m_guitar );
    }

}

bool
Fingering::operator== ( Fingering const& rhs ) const
{
    std::list<unsigned int> pressedStringList;
    bool result = true;

    // Comparison - match guitars
    if ( ! ( *m_guitar == *rhs.m_guitar ) ) {
        result = false;
    }

    if ( m_notes.size() != rhs.m_notes.size() ) {
        result = false;
    }

    if ( result ) {
        // Comparison - Match lhs and rhs notes
        // Worse case: O(N)
        NoteMap::const_iterator lhsPos = m_notes.begin();
        NoteMap::const_iterator rhsPos = rhs.m_notes.begin();

        while ( ( lhsPos != m_notes.end() ) &&
                ( rhsPos != rhs.m_notes.end() ) &&
                result ) {
            Note const * lhsNotePtr = ( *lhsPos ).second;
            Note const* rhsNotePtr = ( *rhsPos ).second;
            if ( ! ( ( *lhsNotePtr ) == ( *rhsNotePtr ) ) ) {
                result = false;
            }

            ++lhsPos;
            ++rhsPos;

        }
    }

    if ( m_barreFretMap.size() != rhs.m_barreFretMap.size() ) {
        result = false;
    }

    if ( result ) {
        // Comparison #4 - Match lhs and rhs barres
        // Worse case: O(N)
        BarreMap::const_iterator lhsPos = m_barreFretMap.begin();
        BarreMap::const_iterator rhsPos = rhs.m_barreFretMap.begin();

        while ( ( lhsPos != m_barreFretMap.end() ) &&
                ( rhsPos != rhs.m_barreFretMap.end() ) &&
                result ) {
            Barre * lhsBarrePtr = ( *lhsPos ).second;
            Barre* rhsBarrePtr = ( *rhsPos ).second;
            if ( !( *lhsBarrePtr == *rhsBarrePtr ) ) {
                return false;
            }
            ++rhsPos;
            ++lhsPos;
        }
    }

    return result;
}

Event*
Fingering::getAsEvent ( timeT absoluteTime )
{
    // Add Fretboard information to the event
    Event * e_ptr =
        new Event ( Fingering::EventType, absoluteTime, 0, EventSubOrdering );

    // - Save guitar
    e_ptr->set
    <UInt>( PropertyName( "GUITAR_MAXFRETS" ),
            m_guitar->getNumberOfFrets() );
    e_ptr->set
    <UInt>( PropertyName( "GUITAR_MAXSTRINGS" ),
            m_guitar->getNumberOfStrings() );

    // - Add start fret
    e_ptr->set
    <UInt>( PropertyName( "STARTFRET" ),
            m_startFret );

    // - Add number of Notes to event
    unsigned int note_size = m_notes.size();
    e_ptr->set
    <UInt>( PropertyName( "NOTE_NUM" ), note_size );

    unsigned int i = 1;
    // - For x in NoteList
    for ( NoteMap::const_iterator n_pos = m_notes.begin();
            n_pos != m_notes.end();
            ++n_pos ) {
        Note const* n_ptr = ( *n_pos ).second;

        QString name = QString( "NOTE%1.%2" )
                       .arg( i )
                       .arg( "STRING" );
        e_ptr->set
        <UInt>( PropertyName( name.ascii() ),
                n_ptr->getStringNumber() );

        name = QString( "NOTE%1.%2" )
               .arg( i )
               .arg( "FRET" );
        e_ptr->set
        <UInt>( PropertyName( name.ascii() ),
                n_ptr->getFret() );

        //   - Add name + value to event
        GuitarString::Action note_act =
            getStringStatus( n_ptr->getStringNumber() );

        name = QString( "NOTE%1.%2" )
               .arg( i )
               .arg( "ACTION" );
        e_ptr->set
        <UInt>( PropertyName( name.ascii() ),
                note_act );

        ++i;
    }

    // - Add number of Barres to event
    unsigned int barre_size = m_barreFretMap.size();
    e_ptr->set
    <UInt>( PropertyName( "BARRE_NUM" ),
            barre_size );

    // - For x in BarreList
    i = 1;
    for ( BarreMap::const_iterator b_pos = m_barreFretMap.begin();
            b_pos != m_barreFretMap.end();
            ++b_pos ) {
        Barre const* b_ptr = ( *b_pos ).second;

        QString name = QString( "BARRE%1.%2" )
                       .arg( i )
                       .arg( "FRET" );
        e_ptr->set
        <UInt>( PropertyName( name.ascii() ),
                b_ptr->getFret() );

        name = QString( "BARRE%1.%2" )
               .arg( i )
               .arg( "START" );
        e_ptr->set
        <UInt>( PropertyName( name.ascii() ),
                b_ptr->getStart() );

        name = QString( "BARRE%1.%2" )
               .arg( i )
               .arg( "END" );
        e_ptr->set
        <UInt>( PropertyName( name.ascii() ),
                b_ptr->getEnd() );

        ++i;
    }
    return e_ptr;
}

}

}
