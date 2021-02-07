<?php

/*
  Plugin Name: Head, Footer and Post Injections
  Plugin URI: http://www.satollo.net/plugins/header-footer
  Description: Header and Footer lets to add html/javascript code to the head and footer and posts of your blog. Some examples are provided on the <a href="http://www.satollo.net/plugins/header-footer">official page</a>.
  Version: 3.2.1
  Author: Stefano Lissa
  Author URI: http://www.satollo.net
  Disclaimer: Use at your own risk. No warranty expressed or implied is provided.
 */

/*
  Copyright 2008-2020 Stefano Lissa (stefano@satollo.net)

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

defined('ABSPATH') || exit;

$hefo_options = get_option('hefo', []);

$hefo_is_mobile = false;
if (isset($_SERVER['HTTP_USER_AGENT']) && isset($hefo_options['mobile_user_agents_parsed'])) {
    $hefo_is_mobile = preg_match('/' . $hefo_options['mobile_user_agents_parsed'] . '/', strtolower($_SERVER['HTTP_USER_AGENT']));
}

if (is_admin()) {
    require_once dirname(__FILE__) . '/admin/admin.php';
}

if (isset($hefo_options['disable_css_id'])) {

    function hefo_style_loader_tag($link) {
        global $hefo_options;
        $link = preg_replace("/id='.*?-css'/", "", $link);
        if (isset($hefo_options['disable_css_media'])) {
            if (!preg_match("/media='print'/", $link)) {
                $link = preg_replace("/media='.*?'/", "", $link);
            }
        }
        return $link;
    }

    add_filter('style_loader_tag', 'hefo_style_loader_tag');
}

register_activation_hook(__FILE__, function () {
    $options = get_option('hefo');
    if (!is_array($options)) {
        $options = [];
    }
    $options = array_merge(['after' => '', 'before' => '', 'head' => '', 'body' => '', 'head_home' => '', 'footer' => ''], $options);
    for ($i = 1; $i <= 5; $i++) {
        $options['snippet_' . $i] = '';
        $options['generic_' . $i] = '';
    }
    $options['updated'] = time(); // Force an update if the old options match (otherwise the autoload is not saved)
    update_option('hefo', $options, true);
});

register_deactivation_hook(__FILE__, function () {
    $options = get_option('hefo');
    if ($options) {
        $options['updated'] = time();
        update_option('hefo', $options, false);
    }
});

add_action('template_redirect', 'hefo_template_redirect', 1);

$hefo_body_block = '';
$hefo_generic_block = array();

function hefo_template_redirect() {
    global $hefo_body_block, $hefo_generic_block, $hefo_options, $hefo_is_mobile;

    if (function_exists('is_amp_endpoint') && is_amp_endpoint())
        return;

    if ($hefo_is_mobile && isset($hefo_options['mobile_body_enabled'])) {
        $hefo_body_block = hefo_execute_option('mobile_body');
    } else {
        $hefo_body_block = hefo_execute_option('body');
    }

    for ($i = 1; $i <= 5; $i++) {
        if ($hefo_is_mobile && isset($hefo_options['mobile_generic_enabled_' . $i])) {
            $hefo_generic_block[$i] = hefo_execute_option('mobile_generic_' . $i);
        } else {
            $hefo_generic_block[$i] = hefo_execute_option('generic_' . $i);
        }
    }

    ob_start('hefo_callback');
}

function hefo_callback($buffer) {
    global $hefo_body_block, $hefo_generic_block, $hefo_options, $hefo_is_mobile;

    for ($i = 1; $i <= 5; $i++) {
        if (isset($hefo_options['generic_tag_' . $i]))
            hefo_insert_before($buffer, $hefo_generic_block[$i], $hefo_options['generic_tag_' . $i]);
    }
    $x = strpos($buffer, '<body');
    if ($x === false) {
        return $buffer;
    }
    $x = strpos($buffer, '>', $x);
    if ($x === false) {
        return $buffer;
    }
    $x++;
    return substr($buffer, 0, $x) . "\n" . $hefo_body_block . substr($buffer, $x);
}

add_action('wp_head', 'hefo_wp_head_pre', 1);

function hefo_wp_head_pre() {
    global $hefo_options, $wp_query;

    if (isset($hefo_options['disable_wlwmanifest_link'])) {
        remove_action('wp_head', 'wlwmanifest_link');
    }

    if (isset($hefo_options['disable_rsd_link'])) {
        remove_action('wp_head', 'rsd_link');
    }

    if (isset($hefo_options['disable_feed_links_extra'])) {
        remove_action('wp_head', 'feed_links_extra', 3);
    }

    if (isset($hefo_options['disable_wp_shortlink_wp_head'])) {
        remove_action('wp_head', 'wp_shortlink_wp_head', 10, 0);
    }

    if (isset($hefo_options['disable_wp_shortlink_wp_head'])) {
        remove_action('wp_head', 'wp_shortlink_wp_head', 10, 0);
    }
}

add_action('wp_head', 'hefo_wp_head_post', 11);

function hefo_wp_head_post() {
    if (is_front_page()) {
        hefo_execute_option('head_home', true);
    }

    hefo_execute_option('head', true);
}

add_action('amp_post_template_head', function () {
    echo hefo_execute_option('amp_head', true);
}, 100);


add_action('amp_post_template_css', function () {
    hefo_execute_option('amp_css', true);
}, 100);

add_action('amp_post_template_body_open', function () {
    hefo_execute_option('amp_body', true);
}, 100);

add_action('amp_post_template_footer', function () {
    hefo_execute_option('amp_footer', true);
}, 100);

add_action('wp_footer', 'hefo_wp_footer');

function hefo_wp_footer() {
    global $hefo_is_mobile;

    if ($hefo_is_mobile && isset($hefo_options['mobile_footer_enabled'])) {
        hefo_execute_option('mobile_footer', true);
    } else {
        hefo_execute_option('footer', true);
    }
}

add_action('the_content', 'hefo_the_content');

function hefo_the_content($content) {
    global $hefo_options, $wpdb, $post, $hefo_is_mobile;

    $before = '';
    $after = '';

    // AMP detection
    if (function_exists('is_amp_endpoint') && is_amp_endpoint()) {
        $before = hefo_execute_option('amp_post_before');
        $after = hefo_execute_option('amp_post_after');
        return $before . $content . $after;
    }

    if (!is_singular()) {
        return $content;
    }
    $type = '';

    if (is_page() && !isset($hefo_options['page_use_post'])) {
        $type = 'page_';
    }

    if (!get_post_meta($post->ID, 'hefo_before', true)) {
        if ($hefo_is_mobile && isset($hefo_options['mobile_' . $type . 'before_enabled'])) {
            $before = hefo_execute_option('mobile_' . $type . 'before');
        } else {
            $before = hefo_execute_option($type . 'before');
        }
    }

    if (!get_post_meta($post->ID, 'hefo_after', true)) {
        if ($hefo_is_mobile && isset($hefo_options['mobile_' . $type . 'after_enabled'])) {
            $after = hefo_execute_option('mobile_' . $type . 'after');
        } else {
            $after = hefo_execute_option($type . 'after');
        }
    }

    // Rules

    for ($i = 1; $i <= 5; $i++) {
        if (empty($hefo_options['inner_tag_' . $i])) {
            continue;
        }
        $prefix = '';
        if ($hefo_is_mobile && isset($hefo_options['mobile_inner_enabled_' . $i])) {
            $prefix = 'mobile_';
        }
        $skip = trim($hefo_options['inner_skip_' . $i]);
        if (empty($skip)) {
            $skip = 0;
        } else if (substr($skip, -1) == '%') {
            $skip = (intval($skip) * strlen($content) / 100);
        }

        if ($hefo_options['inner_pos_' . $i] == 'after') {
            $res = hefo_insert_after($content, hefo_execute_option($prefix . 'inner_' . $i), $hefo_options['inner_tag_' . $i], $skip);
        } else {
            $res = hefo_insert_before($content, hefo_execute_option($prefix . 'inner_' . $i), $hefo_options['inner_tag_' . $i], $skip);
        }
        if (!$res) {
            switch ($hefo_options['inner_alt_' . $i]) {
                case 'after':
                    $content = $content . hefo_execute_option($prefix . 'inner_' . $i);
                    break;
                case 'before':
                    $content = hefo_execute_option($prefix . 'inner_' . $i) . $content;
            }
        }
    }

    return $before . $content . $after;
}

function hefo_insert_before(&$content, $what, $marker, $starting_from = 0) {
    if (strlen($content) < $starting_from) {
        return false;
    }

    if (empty($marker)) {
        $marker = ' ';
    }
    $x = strpos($content, $marker, $starting_from);
    if ($x !== false) {
        $content = substr_replace($content, $what, $x, 0);
        return true;
    }
    return false;
}

function hefo_insert_after(&$content, $what, $marker, $starting_from = 0) {

    if (strlen($content) < $starting_from) {
        return false;
    }

    if (empty($marker)) {
        $marker = ' ';
    }

    $x = strpos($content, $marker, $starting_from);

    if ($x !== false) {
        $content = substr_replace($content, $what, $x + strlen($marker), 0);
        return true;
    }
    return false;
}

add_action('the_excerpt', 'hefo_the_excerpt');
global $hefo_count;
$hefo_count = 0;

function hefo_the_excerpt($content) {
    global $hefo_options, $post, $wpdb, $hefo_count;
    $hefo_count++;
    if (is_category() || is_tag()) {
        $before = hefo_execute_option('excerpt_before');
        $after = hefo_execute_option('excerpt_after');

        return $before . $content . $after;
    } else {
        return $content;
    }
}

function hefo_replace($buffer) {
    global $hefo_options, $post;

    for ($i = 1; $i <= 5; $i++) {
        if (empty($hefo_options['snippet_' . $i]))
            continue;
        $buffer = str_replace('[snippet_' . $i . ']', $hefo_options['snippet_' . $i], $buffer);
    }

    return $buffer;
}

function hefo_execute($buffer) {
    global $post;

    if (apply_filters('hefo_php_exec', true)) {
        ob_start();
        eval('?>' . $buffer);
        $buffer = ob_get_clean();
    }
    return $buffer;
}

function hefo_execute_option($key, $echo = false) {
    global $hefo_options, $wpdb, $post;
    if (empty($hefo_options[$key]))
        return '';
    $buffer = hefo_replace($hefo_options[$key]);
    if ($echo)
        echo hefo_execute($buffer);
    else
        return hefo_execute($buffer);
}
