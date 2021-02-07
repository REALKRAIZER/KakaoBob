<?php
/**
 *
 *  * @ author ( Zikiy Vadim )
 *  * @ site http://online-services.org.ua
 *  * @ name
 *  * @ copyright Copyright (C) 2016 All rights reserved.
 *
 */

//if uninstall/delete not called from WordPress exit
if( ! defined( 'ABSPATH' ) && ! defined( 'WP_UNINSTALL_PLUGIN' ) )
    exit ();

//Удаляем все опции сохраненные плагином
delete_option( 'vdz_yandex_metrika_code' );
delete_option( 'vdz_yandex_metrika_front_section' );
