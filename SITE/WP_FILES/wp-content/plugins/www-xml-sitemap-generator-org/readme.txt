=== Google XML Sitemap Generator ===
Contributors: XmlSitemapGenerator 
Tags: google, google sitemaps, seo, xml sitemap
Donate link: https://xmlsitemapgenerator.org/contribute/subscribeother.aspx?service=wordpress
Requires at least: 4.0
Tested up to: 5.4.0
Stable tag: 1.3.5

Improve your websites SEO with a comprehensive, easy to use RSS and XML sitemap plugin. Compatible with Google, Bing, Baidu, Yandex and more.
 
== Description ==

Improve your websites SEO (Search Engine Optimization) and SERP (Search Engine Page Ranking) using an XML Sitemap.

Originally designed by Google, XML Sitemaps help search engines understand the content of your website, including Google, Bing, Baidu, Yandex and more

[Google XML Sitemap Generator](https://xmlsitemapgenerator.org/wordpress-sitemap-generator-plugin.aspx) adds powerful and configurable RSS and XML sitemap features to your website. 

It works across your entire website creating a detailed sitemap, including all custom posts and taxonomy, which makes it a great companion for plugins such as WooCommerce, bbPress, BuddyPress and more.

A distinct advantage of this plugin is the ability to edit your sitemap defaults for taxonomy and post types, as well as being able to edit individual page and post settings.

**Features include:**

* HTML, RSS and Google XML Sitemap formats.
* List all your WordPress Pages, Posts, Archives, Authors, Categories and Tags.
* Support for custom post types and taxonomy.
* Includes paged links for posts for Archives, Authors, Categories and Tags.
* Set global sitemap default values for priority and update frequencies.
* Set sitemap value for priority and frequency at the category, tag, post and page level.
* Automatic daily auto ping when you have updated pages / posts in WordPress.
* Add latest pages / posts RSS feed to page header.
* Automatically adds sitemap entries to your robots.txt file.
* Add custom entries to your robots.txt file.
* Compatible with WooCommerce, bbPress, BuddyPress and more.

**New features**

We're always seeking to improve and update. 
Stay in touch via [facebook](https://www.facebook.com/XmlSitemapGenerator) and [Twitter](https://twitter.com/createsitemaps)

== Frequently Asked Questions ==

= How easy is it to setup? =

Very easy! Simply install via the wordpress plugin library and activate it.

Go to the XML sitemap settings page and if we alert you to any problems with permalinks, etc. follow the simple instructions to fix these.

That's it, you're up and running.

= Where can I find my sitemap / robots.txt files? =

We create these pages dynamically so you will not find them in your control panel / file system.

Go to the XML sitemap settings page and on the right hand side you will see a list of links to the various pages we produce.

= Why do I get a blank page or 404 error? =

This usually happens when your permalinks are not set up correctly.

We will alert you to this when you go to the XML sitemap settings page.

Simply go to the Permalinks settings page and make sure you are NOT using plain links.

= Why does google think my sitemap is HTML? =

You may have submitted the wrong link. It should be www.yourwebsiteaddress.something/xmlsitemap.xml

Another reason this happens is when your permalinks are not set up correctly.

We will alert you to this when you go to the XML sitemap settings page.

Simply go to the permalinks settings page and make sure you are NOT using plain links.

= Why does google say my website / pages are blocked? =

This is usually because of a problem with your robots.txt file or Wordpress is adding a noindex nofollow meta tag.

This is a problem with your "Search Engine Visibility" on the "Reading settings" page.

We will alert you to this when you go to the XML sitemap settings page.

Simply go to the Reading Settings page and ensure "Discourage search engines from indexing this site" is NOT ticked.

= I'm still having problems what can I do? =

* Check out our [trouble shooting](http://blog.xmlsitemapgenerator.org/2016/06/troubleshooting-your-wordpress-sitemap.html) blog post.
* If you are still stuck get in touch via the settings page and we'll try to help.


== Installation ==

1. We recommend you install Google XML Sitemap Generator via the WordPress.org plugin directory.
2. After activating it usually just works!
3. To make sure click the "Settings" menu and select "XML Sitemap".
4. If you get any alerts about permalinks and search engine visibility, follow the links to fix these.

You should now be up and running, but you may also want to :

5. Change the sitemap global defaults for pages, posts, etc.
4. Configure tag, category, post and page level settings when adding / editing them.

**Still having problems?**

* Check out our [trouble shooting](http://blog.xmlsitemapgenerator.org/2016/06/troubleshooting-your-wordpress-sitemap.html) blog post.

== Screenshots ==

1. Global sitemap settings
2. Post / page sitemap settings
3. Category / tag settings
4. XML Sitemap page
5. Sitemap entries in Robots.txt


== Changelog ==

= 1.3.5 =
Release Date: July 11th, 2020

* New : Tested up to wordpress 5.4.
* New : Edit the sitemap filenames
* New : Set sitemap values per author/user.
* New : Include scheduled posts.
* Fix : Authors variable undefined debug output
* Fix : Page options do not work for multisite/network
* Fix : Incorrect sitemap values due to ID collisions
* Fix : Settings link on network plugins
* Fix : Google ping url

= 1.3.4 =
Release Date: February 28th, 2017

* Fix : Archive reporting incorrect number of pages

= 1.3.2 and 1.3.3 =
Release Date: February 3rd, 2017

* New : Warning if your website is hidden from search engines
* New : Warning about permalink settings
* Fix : Non escaped urls cause invalid sitemap

= 1.3.1 =
Release Date: January 26th, 2017

* Fix : Custom taxonomy, tags & categories not displaying

= 1.3.0 =
Release Date: September 7th, 2016

* New : Support for custom post types and taxonomy
* New : Add custom entries to you robot.txt file.


= 1.2.3 =
Release Date: April 5th, 2016

* Fix : Network / multi-site activation issue
* Fix : robots.txt bug calling addRobotLinks()
* Fix : Errors due to conflicts with some plugin's

= 1.2.2 =
Release Date: December 16th, 2015

* Fix : Duplicate homepage when using a static page

= 1.2.1 =
Release Date: September 23rd, 2015

* Fix : Unexpected out put when activating
* Fix : Missing Hourly update frequency
* Fix : PHP warnings in sitemaps when in debug mode

= 1.2 =
Release Date: September 2nd, 2015

* Fix : invalid sitemap due to default value

= 1.1 =
Release Date: July 27nd, 2015

* Fix : RSS sitemap date format issue
* Fix : XML sitemap date issue
* Fix : Pagination counting pages
* Fix : Logging null value error
* Fix : Ping issues

= 1.0 =
Release Date: July 23rd, 2015
Release Post: http://blog.xmlsitemapgenerator.org/2015/07/wordpress-xml-sitemap-generator.html

* Initial release

