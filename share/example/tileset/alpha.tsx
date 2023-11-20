<?xml version="1.0" encoding="UTF-8"?>
<tileset version="1.10" tiledversion="1.10.2" name="alpha" tilewidth="16" tileheight="16" tilecount="456" columns="24">
 <image source="../img/alpha.png" width="384" height="304"/>
 <tile id="237">
  <properties>
   <property name="library" value="alpha_platform"/>
  </properties>
  <objectgroup draworder="index" id="2">
   <object id="1" name="platform" type="boundary" x="0" y="0" width="16" height="16"/>
  </objectgroup>
 </tile>
 <tile id="274">
  <animation>
   <frame tileid="274" duration="2000"/>
   <frame tileid="322" duration="2000"/>
  </animation>
 </tile>
 <tile id="277">
  <animation>
   <frame tileid="277" duration="1000"/>
   <frame tileid="280" duration="1000"/>
   <frame tileid="325" duration="1000"/>
  </animation>
 </tile>
</tileset>
