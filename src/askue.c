#ifndef _POSIX_SOURCE
    #define _POSIX_SOURCE
#endif

#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>

#ifndef _ASKUE_DEBUG
    #define ASKUE_FILE_HELP "/etc/askue.help"
    #define ASKUE_FILE_PID "/var/askue.pid"
#else
    #define ASKUE_FILE_HELP "./askue.help"
    #define ASKUE_FILE_PID "./askue.pid"
#endif

static
int is_equal_char ( char c1, char c2, int *Result )
{
    return ( *Result = c1 == c2 );
}

static
int is_equal_str ( const char *str, const char *etalon )
{
    int Result = *etalon == *str;
    
    do {
        etalon++;
        str++;
    } while ( !is_equal_char ( *str, '\0', &Result ) && 
               !is_equal_char ( *etalon, '\0', &Result ) &&
               is_equal_char ( *str, *etalon, &Result ) );
               
    return Result;
}

#define is_start( _STR_ ) \
    is_equal_str ( _STR_, "start" )

#define is_stop( _STR_ ) \
    is_equal_str ( _STR_, "stop" )

#define is_restart( _STR_ ) \
    is_equal_str ( _STR_, "restart" )
    
#define is_reconfigure( _STR_ ) \
    is_equal_str ( _STR_, "reconfigure" )
    
#define is_askue_arg( _STR_ ) \
    ({ _STR_[ 0 ] == '-'; })
    
#define is_help( _STR_ ) \
    is_equal_str ( _STR_, "help" )
  
static
int read_ASKUE_pid ( long int *pid )
{
    FILE *PidFile;
    char Buffer[ 256 ];
    printf ( "Чтение '%s'...\n", ASKUE_FILE_PID );
    PidFile = fopen ( ASKUE_FILE_PID, "r" );
    if ( PidFile == NULL )
    {
        if ( snprintf ( Buffer, 256, "Невозможно открыть '%s'.", ASKUE_FILE_PID ) > 0 )
            perror ( Buffer );
        return -1;
    }
    
    if ( fscanf ( PidFile, "%ld\n", pid ) != 1 )
    {
        if ( snprintf ( Buffer, 256, "Ошибка чтения pid из '%s'.", ASKUE_FILE_PID ) > 0 )
            perror ( Buffer );
        return -1;
    }
    printf ( "Чтение завершено.\n" );
    return fclose ( PidFile );
}
    
static
int say_ASKUE_pid ( void )
{
    long int pid;
    if ( !read_ASKUE_pid ( &pid ) )
    {
        return ( printf ( "АСКУЭ уже запущена. Процесс с pid = %ld\n", pid ) > 0 ) ? 0 : -1;
    }
    else
    {
        return -1;
    }
}


// Добавит параметры
static
void init_argv ( char **dest, char **src, size_t dest_amount, size_t src_amount, size_t offset )
{
    int j = 0;
    for ( int i = 1; i < dest_amount; i++ )
    {
        if ( j < src_amount )
        {
            dest[ i ] = src[ j + offset ]; 
            j++;
        }
        else
        {
            dest[ i ] = NULL;
        }
    }
}

static
int Start_ASKUE_proc ( int argc, char **argv, int argv_offset )
{
    // проверка на повторный запуск
    int Exist = access ( ASKUE_FILE_PID, F_OK );
    if ( Exist == 0 )
    {
        return say_ASKUE_pid ();
    }
    else if ( ( Exist == -1 ) && ( errno == ENOENT ) )
    {
        #define ARGV_AMOUNT 5
        char *Argv[ ARGV_AMOUNT ];
        
        // Обнулить и добавить параметры
        init_argv ( Argv, argv, ARGV_AMOUNT, argc, argv_offset );
        // имя вызываемой программы
        #ifndef _ASKUE_DEBUG
            Argv[ 0 ] = "askue-main";
        #else
            Argv[ 0 ] = "/askue-main";
        #endif
        
        if ( execvp ( Argv[ 0 ], ( char * const * )Argv ) )
        {
            perror ( "Ошибка запуска АСКУЭ." );
        }
        #undef ARGV_AMOUNT
    }
    else
    {
        return -1;
    }
}

static
void wait_exit ( void )
{
    int Exist;
    while ( ( Exist = access ( ASKUE_FILE_PID, F_OK ) ) == 0 );
    if ( ( Exist == -1 ) && ( errno == ENOENT ) )
        puts ( "Работа АСКУЭ завершена." );
}

static
int Stop_ASKUE_proc ( void )
{
    long int pid;
    if ( read_ASKUE_pid ( &pid ) )
    {
        return -1;
    }
    if ( kill ( ( pid_t ) pid, SIGTERM ) == -1 )
    {
        perror ( "Ошибка: Stop_ASKUE_proc(): kill()" );
    }
    
    wait_exit ();
    
    return 0;
}

static
int Help_ASKUE_proc ( void )
{
    FILE *Help;
    
    Help = fopen ( ASKUE_FILE_HELP, "r" );
    if ( Help == 0 )
    {
        perror ( "Невозможно открыть '/etc/askue.help'." );
        return -1;
    }
    
    while ( !feof ( Help ) )
    {
        char Buffer[ 512 ];
        size_t BufLen = fread ( Buffer, sizeof ( char ), 512, Help );
        fwrite ( Buffer, sizeof ( char ), BufLen, stdout );
    }
    
    fclose ( Help );
    
    return 0;
}

#define ReStart_ASKUE_proc( argc, argv, argv_offser )\
({ ( !Stop_ASKUE_proc() ) ? Start_ASKUE_proc ( argc, argv, argv_offser ) : -1; })

static
int ReConfigure_ASKUE_proc ( void )
{
    long int pid;
    if ( read_ASKUE_pid ( &pid ) )
    {
        return -1;
    }
    if ( kill ( ( pid_t ) pid, SIGUSR1 ) == -1 )
    {
        perror ( "Ошибка: ReConfigure_ASKUE_proc(): kill()" );
        return -1;
    }
    puts ( "АСКУЭ переконфигурирована." );
    
    return 0;
}

static
void Error_ASKUE_proc ( void )
{
    puts ( "" );
    puts ( "Указаны неправильные аргумент." );
    puts ( "" );
    puts ( "Используйте:" );
    puts ( "" );
    puts ( "askue start <опции_демона>" );
    puts ( "    для запуска АСКУЭ" );
    puts ( "askue stop" );
    puts ( "    для остановки АСКУЭ" );
    puts ( "askue restart <опции демона>" );
    puts ( "    для перезапуска АСКУЭ с новыми параметрами" );
    puts ( "askue reconfigure" );
    puts ( "    для переконфигурации АСКУЭ" );
    puts ( "askue help" );
    puts ( "    для вывода справки на экран" );
    puts ( "" );
    puts ( "Don’t use the Force, Luke, try to think!" );
    puts ( "" );
}


int main(int argc, char **argv)
{
	if ( argc == 1 )
    {
        return Start_ASKUE_proc ( argc - 1, argv, 1 );
    }
    else if ( is_start ( argv[ 1 ] ) )
    {
        return Start_ASKUE_proc ( argc - 2, argv, 2 );
    }
    else if ( is_stop ( argv[ 1 ] ) )
    {
        return Stop_ASKUE_proc ();
    }
    else if ( is_restart ( argv[ 1 ] ) )
    {
        return ReStart_ASKUE_proc ( argc - 2, argv, 2 );
    }
    else if ( is_reconfigure ( argv[ 1 ] ) )
    {
        return ReConfigure_ASKUE_proc ();
    }
    else if ( is_askue_arg ( argv[ 1 ] ) )
    {
        return Start_ASKUE_proc ( argc - 1, argv, 1 );
    }
    else if ( is_help ( argv[ 1 ] ) )
    {
        return Help_ASKUE_proc ();
    }
    else
    {
        Error_ASKUE_proc ();
        return -1;
    }
}

#undef ReStart_ASKUE_proc
#undef is_start
#undef is_stop
#undef is_restart
#undef is_reconfigure
#undef is_askue_arg
#undef is_help
