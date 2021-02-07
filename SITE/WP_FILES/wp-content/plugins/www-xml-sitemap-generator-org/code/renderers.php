<?php

namespace xmlSitemapGenerator;

	interface iSitemapRenderer
	{
		public function render($urls);
	}

	class sitemapRenderer  
	{
		
		static function getInstance($type)
		{
			switch ($type) 
			{
				case 'html':
					return new htmlRenderer();
				case 'rss':
					return new rssRenderer();
				case 'xml':
					return new xmlRenderer();
				case 'xsl':
					return new xslRenderer();
				default:
					return null;
			}
		}
	}
	
	class rssRenderer implements iSitemapRenderer
	{
		
		function renderItem( $url)
		{
			
			 echo '<item>'  ;
				echo '<guid>'  . htmlspecialchars($url->location) . '</guid>';
				echo '<title>'  . $url->title . '</title>';
				echo '<link>'  . htmlspecialchars($url->location) . '</link>';
				echo '<description>' . $url->description . '</description>';
				echo '<pubDate>' . date(DATE_RSS, $url->modified) . '</pubDate>';
			 echo "</item>\n" ;
		}
		
		function render($urls){
			
		  	ob_get_clean();
		 	ob_start();
			header('Content-Type: text/xml; charset=utf-8');
			
			echo '<?xml version="1.0" encoding="UTF-8" ?>';
			echo  "\n";
			sitemapBuilder::renderComment();
			echo  "\n";
			echo  '<rss version="2.0">';
			echo  "\n";
			echo  '<channel>';
			echo  "\n";
	

											
			echo '<title>'  . get_option('blogname') . '</title>';
			echo '<link>'  . get_bloginfo( 'url' ) . '</link>';
			echo '<description>' . get_option( 'blogdescription'). '</description>';
			
			echo  "\n";
			foreach( $urls as $url ) 
			{
				
				$this->renderItem($url);
			}	
			echo  "\n";
			echo '</channel>';
			echo  "\n";
			echo '</rss>';
			echo  "\n";
			sitemapBuilder::renderComment();
			echo  "\n";
			ob_end_flush();
						
		}
		
		
	}
	
	class xmlRenderer implements iSitemapRenderer
	{

	//	$writer;// = new XMLWriter();  
	 
		function getFrequency($value)
		{
			switch ($value) {
				case 0:
					return "";
					break;
				case 1:
					return "default";
					break;
				case 2:
					return "never";
					break;
				case 3:
					return "yearly";
					break;
				case 4:
					return "monthly";
					break;
				case 5:
					return "weekly";
					break;
				case 6:
					return "daily";
					break;
				case 7:
					return "hourly";
					break;
				case 8:
					return "always";
					break;
				default:
					return "xxx";
			}
		}

		function getPriority($value)
		{
			switch ($value) {
				case 0:
					return "";
					break;
				case 1:
					return "default";
					break;
				case 2:
					return "0.0";
					break;
				case 3:
					return "0.1";
					break;
				case 4:
					return "0.2";
					break;
				case 5:
					return "0.3";
					break;
				case 6:
					return "0.4";
					break;
				case 7:
					return "0.5";
					break;
				case 8:
					return "0.6";
					break;
				case 9:
					return "0.7";
					break;
				case 10:
					return "0.8";
					break;
				case 11:
					return "0.9";
					break;
				case 12:
					return "1.0";
					break;
				default:
					return "xxx";
			}
		}
		
		function renderItem( $url)
		{
			
			 echo '<url>'  ;
				echo '<loc>'  . htmlspecialchars($url->location) . '</loc>';
				 echo '<lastmod>' . date('Y-m-d\TH:i:s+00:00', $url->modified) . '</lastmod>';
				 
				 if (!$url->frequency==0) {
					 echo '<changefreq>' .  $this->getFrequency($url->frequency) . '</changefreq>';
				 }
				 
				 if (!$url->priority==0) {
					 echo "<priority>" . $this->getPriority($url->priority) . "</priority>";
				 } 
				 
			 echo "</url>\n" ;
		}
		
		function render($urls){
			
		  	ob_get_clean();
			$url = get_bloginfo( 'url' ) . '/xmlsitemap.xsl';
		
		 	ob_start();
			header('Content-Type: text/xml; charset=utf-8');
			
			echo '<?xml version="1.0" encoding="UTF-8" ?>';
			echo  "\n";
			echo '<?xml-stylesheet type="text/xsl" href="' . $url . '"?>';
			echo  "\n";
			sitemapBuilder::renderComment();
			echo  "\n";
			echo  '<urlset xmlns="http://www.sitemaps.org/schemas/sitemap/0.9">';
			echo  "\n";
			foreach( $urls as $url ) 
			{
				$this->renderItem($url);
			}	
			echo  "\n";
			echo '</urlset>';
			echo  "\n";
			sitemapBuilder::renderComment();
			echo  "\n";
			ob_end_flush();
	
		}
		
		
	}
	
	
	class htmlRenderer implements iSitemapRenderer
	{
		
		
		function renderItem( $url)
		{
			
			 echo '<li>'  ;
				echo '<a href="' . htmlspecialchars($url->location) . '">';
				echo $url->title;
				echo '</a>';
			 echo "</li>\n" ;
		}
		
		function render($urls){
			
		  	ob_get_clean();
			header('Content-Type: text/html; charset=utf-8');
		 	ob_start();
			
			
			self::renderHeader();
			
			echo  "<ul>\n";
			foreach( $urls as $url ) 
			{
				
				$this->renderItem($url);
			}	
			echo  "</ul>\n";
			sitemapBuilder::renderCredit();
			self::renderFooter();
			echo  "\n";
			sitemapBuilder::renderComment();
			echo  "\n";
			ob_end_flush();
			
		}
		
		function renderHeader()
		{
			?>
<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<html xmlns="http://www.w3.org/1999/xhtml">
<!--Created using XmlSitemapGenerator.org WordPress Plugin - Free HTML, RSS and XML sitemap generator -->
<head>
    <title>WordPress Sitemap Generator</title>
    <meta id="MetaDescription" name="description" content="WordPress Sitemap created using XmlSitemapGenerator.org - the free online Google XML sitemap generator" />
    <meta id="MetaKeywords" name="keywords" content="XML, HTML, Sitemap Generator, Wordpress" />
    <meta content="Xml Sitemap Generator .org" name="Author" />
    <style>
        body, head, #xsg {margin:0px 0px 0px 0px; line-height:22px; color:#666666; width:100%; padding:0px 0px 0px 0px; 
                          font-family : Tahoma, Verdana,   Arial, sans-serif; font-size:13px;}
        
        #xsg ul li a {font-weight:bold; }
        #xsg ul ul li a {font-weight:normal; }
        #xsg a {text-decoration:none; }
        #xsg p {margin:10px 0px 10px 0px;}
        #xsg ul {list-style:square; }
        #xsg li {}
        #xsg th { text-align:left;font-size: 0.9em;padding:2px 10px 2px 2px; border-bottom:1px solid #CCCCCC; border-collapse:collapse;}
        #xsg td { text-align:left;font-size: 0.9em; padding:2px 10px 2px 2px; border-bottom:1px solid #CCCCCC; border-collapse:collapse;}
        
        #xsg .title {font-size: 0.9em;  color:#132687;  display:inline;}
        #xsg .url {font-size: 0.7em; color:#999999;}
        
        #xsgHeader { width:100%; float:left; margin:0px 0px 5px 0px; border-bottom:2px solid #132687; }
        #xsgHeader h1 {  padding:0px 0px 0px 20px ; float:left;}
        #xsgHeader h1 a {color:#132687; font-size:14px; text-decoration:none;cursor:default;}
        #xsgBody {padding:10px;float:left;}
        #xsgFooter { color:#999999; width:100%; float:left; margin:20px 0px 15px 0px; border-top:1px solid #999999;padding: 10px 0px 10px 0px; }
        #xsgFooter a {color:#999999; font-size:11px; text-decoration:none;   }    
        #xsgFooter span {color:#999999; font-size:11px; text-decoration:none; margin-left:20px; }  
    </style>
</head>
<body>
<div id="xsg">
<div id="xsgHeader">
    <h1><a href="#" title="Wordpress">Wordpress Sitemap</a></h1>
</div>
<div id="xsgBody">
			
		<?php 
		}
		
		
		static function renderFooter()
		{
			?>
		
</div>



</body>
</html>

		<?php
	}
		
}
		
	class xslRenderer implements iSitemapRenderer
	{
		public function render($urls){
			
		  	ob_get_clean();
			header('Content-Type: text/xsl; charset=utf-8');
		 	ob_start();
			
			
			self::output();
			
			
		 
			echo  "\n";
			ob_end_flush();
			
		}
		
		function output()
		{  
			echo '<?xml version="1.0" encoding="UTF-8"?>';
?>
<xsl:stylesheet version="1.0"   xmlns:html="http://www.w3.org/TR/REC-html40"  xmlns:sitemap="http://www.sitemaps.org/schemas/sitemap/0.9" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
<xsl:output method="html" version="1.0" encoding="UTF-8" indent="yes" />

<xsl:template match="/">
<html xmlns="http://www.w3.org/1999/xhtml">
<head>
<title>Wordpress XML Sitemap Generator Plugin</title>
<meta http-equiv="Content-Type" content="text/html; charset=utf-8" />
<meta name="robots" content="index,follow" />
<style type="text/css">
  body {font-family:Tahoma, Verdana, Arial, sans-serif;font-size:12px; }
  #header { padding:0px; margin-top:10px; margin-bottom:20px;}
  a {text-decoration:none; color:blue;}
  table {margin-bottom:50px;}
  tr:nth-child(odd)		{ background-color:#eeeeee; }
  th {font-size:12px; padding:5px;text-align:left; vertical-align:bottom;}
  td {font-size:12px; padding:5px; }
</style>
			</head>
			<body>
				<xsl:apply-templates></xsl:apply-templates>
			</body>
		</html>
	</xsl:template>
	
	
	<xsl:template match="sitemap:urlset">
        <h1>Wordpress XML Sitemap</h1>

    <div id="header">
      <p>
        This is an XML Sitemap comaptible with major search engines such as Google, Bing, Baidu and Yandex.<br />
        For more information and support go to <a href="https://xmlsitemapgenerator.org/wordpress-sitemap-generator-plugin.aspx">Wordpress Sitemap Generator Plugin</a> homepage. <br />
        You can find more information about XML sitemaps on <a href="http://sitemaps.org">sitemaps.org</a>
      </p>
    </div>



    <table cellspacing="0">
        <tr>
          <th>Page url</th>
          <th style="width:80px;">Relative<br />priority</th>
          <th style="width:80px;">Change<br />frequency</th>
          <th style="width:130px;">Modified<br />date</th>
        </tr>
        <xsl:variable name="lower" select="'abcdefghijklmnopqrstuvwxyz'"/>
        <xsl:variable name="upper" select="'ABCDEFGHIJKLMNOPQRSTUVWXYZ'"/>
        <xsl:for-each select="./sitemap:url">
          <tr>
            <xsl:if test="position() mod 2 != 1">
              <xsl:attribute  name="class">high</xsl:attribute>
            </xsl:if>
            <td>
              <xsl:variable name="page">
                <xsl:value-of select="sitemap:loc"/>
              </xsl:variable>
              <a target="_blank" href="{$page}">
                <xsl:value-of select="sitemap:loc"/>
              </a>
            </td>
            <td>
              <xsl:value-of select="sitemap:priority"/>
            </td>
            <td>
              <xsl:value-of select="sitemap:changefreq"/>
            </td>
            <td>
              <xsl:value-of select="sitemap:lastmod"/>
            </td>
          </tr>
        </xsl:for-each>
      </table>
	  
		<?php sitemapBuilder::renderCredit(); ?>
 
	</xsl:template>
	

 
</xsl:stylesheet>
		
<?php		
		}
		
	}

	
	
?>