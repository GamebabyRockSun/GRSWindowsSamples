<?xml version="1.0"?>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
  <xsl:template match="/">
    <html>
      <body>
        <h2>珍藏版CD详细列表</h2>
        <table border="1">
			<tr bgcolor="#9acd32">
            <th align="center">主题</th>
            <th align="center">艺术家</th>
            <th align="center">国家</th>
            <th align="center">出版公司</th>
            <th align="center">价格</th>
            <th align="center">年代</th>
          </tr>
          <xsl:for-each select="catalog/cd">
            <tr>
              <td>
                <xsl:value-of select="title"/>
              </td>
              <td>
                <xsl:value-of select="artist"/>
              </td>
              <td>
                <xsl:value-of select="country"/>
              </td>
              <td>
                <xsl:value-of select="company"/>
              </td>
              <td>
                <xsl:value-of select="price"/>
              </td>
              <td>
                <xsl:value-of select="year"/>
              </td>
            </tr>
          </xsl:for-each>
        </table>
      </body>
    </html>
  </xsl:template>
</xsl:stylesheet>