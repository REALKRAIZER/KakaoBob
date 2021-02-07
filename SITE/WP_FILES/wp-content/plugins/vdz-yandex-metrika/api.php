<?php
/**
 * @ author ( Zikiy Vadim )
 * @ site http://online-services.org.ua
 * @ date 30/11/2016
 * @ copyright Copyright (C) 2016 All rights reserved.
 */

//Получаем из API все свои плагины со ссылками на них
add_action(VDZ_YM_API, function($action = '', $plugin_name = ''){
    if(empty($plugin_name)) $plugin_name = plugin_basename(__FILE__);
    if(empty($action) || !in_array($action, array('on', 'off'))) $action = 'on'; // on || off

    //Получаем название папки плагина
    $plugin_name = preg_replace('|\/(.*)|', '', $plugin_name);

    $response = wp_remote_get( "http://api.online-services.org.ua/{$action}/{$plugin_name}");
    if ( ! is_wp_error( $response ) && isset($response['body']) && (json_decode($response['body']) !== null ) ){
        //Сохраняем временные данные сообщения, время отображения 60 секунд
        set_transient( VDZ_YM_API . '_activation_notice_text', $response['body'], 60 );
    }
}, 10, 2);

//Выводим информацию в админке
add_action( 'admin_notices', function () {
    //Получаем временные данные сообщения
    $message = get_transient( VDZ_YM_API . '_activation_notice_text' );
    $all_plugins = json_decode($message, true);

    if( !empty($all_plugins) ){
        $class = 'notice notice-success is-dismissible';
        $msg = '';
        $msg .= '<h3>'.__('Thank you for creating with VDZ plugins.').'</h3>';
        $msg .= '<p>'.__('Maybe you like, and our other plugins?').'</p>';

        $msg .= '<table style="border: none; text-align: center;">';
        $msg .= '<tr><th style="min-width: 150px;">'.__('Plugin name').'</th>
                <th style="min-width: 150px;">'.__('View on Wordpress site').'</th>
                <th style="min-width: 150px;">'.__('View on Admin page').'</th></tr>';
        foreach ($all_plugins as $link => $plugin_name){
            $msg .= '<tr style="line-height: 1.8em;">';
            $msg .= '<td><strong>'.$plugin_name.'</strong></td>';
            $msg .= '<td><a href="'.$link.'" target="_blank">'.__('View').'</a></td>';
            $url_encode_plugin_name = urlencode('"'.trim($plugin_name).'"');
            $url_encode_plugin_url = 'plugin-install.php?tab=search&type=term&s=' . $url_encode_plugin_name;
            $msg .= '<td><a href="'. get_admin_url(null, $url_encode_plugin_url) .'" target="_blank">'.__('View').'</a></td>';
            $msg .= '</tr>';
        }
        $msg .= '</table>';

        printf( '<div id="'.VDZ_YM_API.'" class="%1$s">%2$s</div>', $class, $msg );
        //Мгновенное удаление временного сообщения
        delete_transient( VDZ_YM_API . '_activation_notice_text' );
    }
});
