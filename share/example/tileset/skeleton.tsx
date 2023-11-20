<?xml version="1.0" encoding="UTF-8"?>
<tileset version="1.10" tiledversion="1.10.2" name="skeleton" tilewidth="24" tileheight="48" spacing="1" tilecount="8" columns="8">
 <properties>
  <property name="library" value="skeleton"/>
 </properties>
 <image source="../img/skeleton.png" width="199" height="48"/>
 <tile id="0">
  <properties>
   <property name="name" value="walk"/>
  </properties>
  <objectgroup draworder="index" id="2">
   <object id="1" type="collision" x="7" y="5" width="10" height="43"/>
   <object id="2" type="hitbox" x="7" y="5" width="10" height="43"/>
   <object id="3" type="hurtbox" x="2" y="3" width="18" height="45"/>
  </objectgroup>
  <animation>
   <frame tileid="0" duration="150"/>
   <frame tileid="1" duration="150"/>
   <frame tileid="2" duration="150"/>
   <frame tileid="3" duration="150"/>
   <frame tileid="4" duration="150"/>
   <frame tileid="5" duration="150"/>
   <frame tileid="6" duration="150"/>
   <frame tileid="7" duration="150"/>
  </animation>
 </tile>
 <tile id="1">
  <objectgroup draworder="index" id="2">
   <object id="3" type="hurtbox" x="3" y="2" width="18" height="46"/>
   <object id="1" type="collision" x="7" y="5" width="10" height="43"/>
   <object id="2" type="hitbox" x="8" y="5" width="10" height="43"/>
  </objectgroup>
 </tile>
 <tile id="2">
  <objectgroup draworder="index" id="2">
   <object id="1" type="collision" x="7" y="5" width="10" height="43"/>
   <object id="2" type="hitbox" x="9" y="5" width="8" height="43"/>
   <object id="3" type="hurtbox" x="5" y="2" width="15" height="46"/>
  </objectgroup>
 </tile>
 <tile id="3">
  <objectgroup draworder="index" id="2">
   <object id="1" type="collision" x="7" y="5" width="10" height="43"/>
   <object id="2" type="hitbox" x="8" y="5" width="9" height="43"/>
   <object id="3" type="hurtbox" x="5" y="2" width="14" height="46"/>
  </objectgroup>
 </tile>
 <tile id="4">
  <objectgroup draworder="index" id="2">
   <object id="3" type="hurtbox" x="3" y="3" width="20" height="45"/>
   <object id="1" type="collision" x="7" y="5" width="10" height="43"/>
   <object id="2" type="hitbox" x="6" y="7" width="12" height="41"/>
  </objectgroup>
 </tile>
 <tile id="5">
  <objectgroup draworder="index" id="2">
   <object id="1" type="collision" x="7" y="5" width="10" height="43"/>
   <object id="2" type="hitbox" x="8" y="6" width="9" height="42"/>
   <object id="3" type="hurtbox" x="3" y="2" width="17" height="46"/>
  </objectgroup>
 </tile>
 <tile id="6">
  <objectgroup draworder="index" id="2">
   <object id="1" type="collision" x="7" y="5" width="10" height="43"/>
   <object id="2" type="hitbox" x="9" y="6" width="9" height="42"/>
   <object id="3" type="hurtbox" x="5" y="2" width="15" height="46"/>
  </objectgroup>
 </tile>
 <tile id="7">
  <objectgroup draworder="index" id="2">
   <object id="3" type="hurtbox" x="4" y="3" width="17" height="45"/>
   <object id="1" type="collision" x="7" y="5" width="10" height="43"/>
   <object id="2" type="hitbox" x="8" y="7" width="9" height="41"/>
  </objectgroup>
 </tile>
</tileset>
