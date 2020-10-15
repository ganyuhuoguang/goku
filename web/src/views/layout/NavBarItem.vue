<template>
  <div>
    <template v-for="item in routes">
      <router-link v-if="!item.hidden && (!item.dropdown || item.dropdown &&
      item.children.length===1)" :to="item.path">
        <MenuItem :name="item.path+'/'+item.children[0].path">
          <Icon v-if='item.icon' :type="item.icon"></Icon>
          {{item.name}}
        </MenuItem>
      </router-link>
      <Submenu :name="item.path" v-if="!item.hidden && item.dropdown && item.children.length>1">
        <template slot="title">
          <Icon v-if='item.icon' :type="item.icon"></Icon>
          {{item.name}}
        </template>
        <template v-for="child in item.children">
          <router-link :to="item.path+'/'+child.path">
            <MenuItem :name="item.path+'/'+child.path">
              <Icon v-if='child.icon' :type="child.icon"></Icon>
              {{child.name}}
            </MenuItem>
          </router-link>
        </template>
      </Submenu>
    </template>
  </div>
</template>
<script>
  export default {
    name: 'NavBarItem',
    props: {
      routes: {
        type: Array
      }
    }
  }
</script>
<style>

</style>
