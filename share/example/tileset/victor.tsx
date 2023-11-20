<?xml version="1.0" encoding="UTF-8"?>
<tileset version="1.5" tiledversion="1.7.1" name="victor" tilewidth="50" tileheight="50" spacing="1" margin="1" tilecount="64" columns="8">
 <properties>
  <property name="library" value="victor"/>
 </properties>
 <image source="../img/victor.png" width="409" height="409"/>
 <tile id="0">
  <properties>
   <property name="name" value="walk"/>
  </properties>
  <objectgroup draworder="index" id="2">
   <object id="1" name="body" type="collision" x="20" y="10" width="10" height="40"/>
  </objectgroup>
  <animation>
   <frame tileid="0" duration="120"/>
   <frame tileid="1" duration="120"/>
   <frame tileid="2" duration="120"/>
   <frame tileid="3" duration="120"/>
   <frame tileid="4" duration="120"/>
   <frame tileid="5" duration="120"/>
   <frame tileid="6" duration="120"/>
   <frame tileid="7" duration="120"/>
  </animation>
 </tile>
 <tile id="1">
  <objectgroup draworder="index" id="3">
   <object id="2" name="body" type="collision" x="20" y="10" width="10" height="40"/>
  </objectgroup>
 </tile>
 <tile id="2">
  <objectgroup draworder="index" id="2">
   <object id="1" name="body" type="collision" x="20" y="10" width="10" height="40"/>
  </objectgroup>
 </tile>
 <tile id="3">
  <objectgroup draworder="index" id="2">
   <object id="1" name="body" type="collision" x="20" y="10" width="10" height="40"/>
  </objectgroup>
 </tile>
 <tile id="4">
  <objectgroup draworder="index" id="2">
   <object id="2" name="body" type="collision" x="20" y="10" width="10" height="40"/>
  </objectgroup>
 </tile>
 <tile id="5">
  <objectgroup draworder="index" id="2">
   <object id="1" name="body" type="collision" x="20" y="10" width="10" height="40"/>
  </objectgroup>
 </tile>
 <tile id="6">
  <objectgroup draworder="index" id="2">
   <object id="1" name="body" type="collision" x="20" y="10" width="10" height="40"/>
  </objectgroup>
 </tile>
 <tile id="7">
  <objectgroup draworder="index" id="2">
   <object id="1" name="body" type="collision" x="20" y="10" width="10" height="40"/>
  </objectgroup>
 </tile>
 <tile id="8">
  <properties>
   <property name="name" value="rest"/>
  </properties>
  <objectgroup draworder="index" id="2">
   <object id="1" name="body" type="collision" x="20" y="10" width="10" height="40"/>
  </objectgroup>
  <animation>
   <frame tileid="8" duration="120"/>
   <frame tileid="9" duration="120"/>
   <frame tileid="10" duration="120"/>
   <frame tileid="9" duration="120"/>
  </animation>
 </tile>
 <tile id="9">
  <objectgroup draworder="index" id="2">
   <object id="2" name="body" type="collision" x="20" y="10" width="10" height="40"/>
  </objectgroup>
 </tile>
 <tile id="10">
  <objectgroup draworder="index" id="2">
   <object id="1" name="body" type="collision" x="20" y="10" width="10" height="40"/>
  </objectgroup>
 </tile>
 <tile id="13">
  <properties>
   <property name="name" value="crouch"/>
  </properties>
  <animation>
   <frame tileid="13" duration="30"/>
   <frame tileid="14" duration="30"/>
   <frame tileid="15" duration="30"/>
  </animation>
 </tile>
 <tile id="16">
  <properties>
   <property name="name" value="jump rise"/>
  </properties>
  <objectgroup draworder="index" id="2">
   <object id="1" name="body" type="collision" x="20" y="10" width="10" height="40"/>
  </objectgroup>
 </tile>
 <tile id="17">
  <properties>
   <property name="name" value="jump fall"/>
  </properties>
  <objectgroup draworder="index" id="2">
   <object id="1" name="body" type="collision" x="20" y="10" width="10" height="40"/>
  </objectgroup>
  <animation>
   <frame tileid="17" duration="30"/>
   <frame tileid="18" duration="30"/>
   <frame tileid="19" duration="30"/>
   <frame tileid="20" duration="30"/>
   <frame tileid="21" duration="30"/>
   <frame tileid="22" duration="30"/>
   <frame tileid="23" duration="30"/>
  </animation>
 </tile>
 <tile id="18">
  <objectgroup draworder="index" id="2">
   <object id="1" name="body" type="collision" x="20" y="10" width="10" height="33"/>
  </objectgroup>
 </tile>
 <tile id="19">
  <objectgroup draworder="index" id="2">
   <object id="1" name="body" type="collision" x="20" y="10" width="10" height="26"/>
  </objectgroup>
 </tile>
 <tile id="20">
  <objectgroup draworder="index" id="2">
   <object id="1" name="body" type="collision" x="20" y="10" width="10" height="31"/>
  </objectgroup>
 </tile>
 <tile id="21">
  <objectgroup draworder="index" id="2">
   <object id="1" name="body" type="collision" x="20" y="10" width="10" height="38"/>
  </objectgroup>
 </tile>
 <tile id="22">
  <properties>
   <property name="name" value="fall"/>
  </properties>
  <objectgroup draworder="index" id="2">
   <object id="1" name="body" type="collision" x="20" y="17" width="10" height="33"/>
  </objectgroup>
  <animation>
   <frame tileid="22" duration="30"/>
   <frame tileid="23" duration="30"/>
  </animation>
 </tile>
 <tile id="23">
  <objectgroup draworder="index" id="2">
   <object id="1" name="body" type="collision" x="20" y="10" width="10" height="40"/>
  </objectgroup>
 </tile>
 <tile id="48">
  <objectgroup draworder="index" id="2">
   <object id="1" name="body" type="collision" x="20" y="10" width="10" height="40"/>
  </objectgroup>
  <animation>
   <frame tileid="48" duration="60"/>
   <frame tileid="49" duration="60"/>
   <frame tileid="50" duration="60"/>
   <frame tileid="51" duration="60"/>
   <frame tileid="52" duration="60"/>
   <frame tileid="53" duration="60"/>
   <frame tileid="54" duration="60"/>
   <frame tileid="55" duration="60"/>
  </animation>
 </tile>
 <tile id="49">
  <objectgroup draworder="index" id="2">
   <object id="1" name="body" type="collision" x="20" y="10" width="10" height="40"/>
  </objectgroup>
 </tile>
 <tile id="50">
  <objectgroup draworder="index" id="2">
   <object id="1" name="body" type="collision" x="20" y="10" width="10" height="40"/>
  </objectgroup>
 </tile>
 <tile id="51">
  <objectgroup draworder="index" id="2">
   <object id="1" name="body" type="collision" x="20" y="10" width="10" height="40"/>
  </objectgroup>
 </tile>
 <tile id="52">
  <objectgroup draworder="index" id="2">
   <object id="1" name="body" type="collision" x="20" y="10" width="10" height="40"/>
  </objectgroup>
 </tile>
 <tile id="53">
  <objectgroup draworder="index" id="2">
   <object id="1" name="body" type="collision" x="20" y="10" width="10" height="40"/>
  </objectgroup>
 </tile>
 <tile id="54">
  <objectgroup draworder="index" id="2">
   <object id="1" name="body" type="collision" x="20" y="10" width="10" height="40"/>
  </objectgroup>
 </tile>
 <tile id="55">
  <objectgroup draworder="index" id="2">
   <object id="1" name="body" type="collision" x="20" y="10" width="10" height="40"/>
  </objectgroup>
 </tile>
</tileset>
