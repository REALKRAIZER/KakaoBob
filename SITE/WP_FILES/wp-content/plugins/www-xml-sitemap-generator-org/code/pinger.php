<?php

namespace xmlSitemapGenerator;

include_once 'dataAccess.php';

class pinger
{

	public static function doPing($tag = "manual")
	{
		$sitemapUrl = urlencode( get_bloginfo( 'url' ) .   '/xmlsitemap.xml');
		$url = "http://www.google.com/webmasters/sitemaps/ping?sitemap=" . $sitemapUrl;
		$response = core::doRequest($url);
		core::statusUpdate("Google {$tag} ping - {$response}");

		$url = "http://www.bing.com/ping?sitemap=" . $sitemapUrl;
		$response = core::doRequest($url);
		core::statusUpdate("Bing {$tag} ping - {$response}");
	
		update_option('xmsg_LastPing', time());
	}
	
	public static function doManualPing()
	{
		 $startTime =  microtime(true) ;
		 self::doPing("Manual");
		 $time = core::getTimeBand($startTime);
		 core::updateStatistics("Ping", "Manual", $time);
		
	}
	public static function doAutoPings($date)
	{
		$startTime =  microtime(true) ;
		$lasModified = dataAccess::getLastModified($date);
		$lastPing = get_option('xmsg_LastPing',0);
		// using UNIX times 
		if ($lastPing < $lasModified )
		{
			self::doPing("Auto");
			update_option('xmsg_LastPing', $lasModified);
		}
		else
		{
			core::statusUpdate("Auto ping skipped. No modified posts");
		}
		 $time = core::getTimeBand($startTime);
		 core::updateStatistics("Ping", "AutoPing", $time);
	}


	
}
?>