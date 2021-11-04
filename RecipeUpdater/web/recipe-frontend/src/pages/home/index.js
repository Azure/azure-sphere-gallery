import React from "react";
import {
  Box, Typography, Grid, Paper, Tooltip, Button
} from "@material-ui/core";
import CloudUploadIcon from '@material-ui/icons/CloudUpload';
import DeviceHubIcon from '@material-ui/icons/DeviceHub';
// import HistoryIcon from '@material-ui/icons/History';
import Table from '@material-ui/core/Table';
import TableBody from '@material-ui/core/TableBody';
import TableCell from '@material-ui/core/TableCell';
import TableContainer from '@material-ui/core/TableContainer';
import TableHead from '@material-ui/core/TableHead';
import TableRow from '@material-ui/core/TableRow';
import Skeleton from '@material-ui/lab/Skeleton';

import BigButton from "./BigButton";
import authFetch from '../../helpers/auth-fetch';
import { authenticationService } from "../../helpers/auth-service";

function isFailedUpdate(row) {
  const { properties } = row;
  const { desired, reported } = properties;

  // just kinda copied this from DeviceTwinPicker, so it ugly af
  if (desired && desired.recipe_campaign) {
    if (reported && reported.recipe_campaign) {
      if (reported.recipe_campaign.failed && desired.recipe_campaign.uuid === reported.recipe_campaign.failed.uuid && desired.recipe_campaign.uuid !== reported.recipe_campaign.uuid) {
        return true;
      }
    }
  }
  return false;
}
export default function Home() {
  const [twins, setTwins] = React.useState(null);
  const controller = new AbortController();
  const [user, setUser] = React.useState(null);

  function fetchTwins() {
    return authFetch(`/api/digital-twins/query?where=${encodeURIComponent('(properties.desired.recipe_campaign.uuid != properties.reported.recipe_campaign.uuid) OR (IS_DEFINED(properties.desired.recipe_campaign.uuid) AND NOT IS_DEFINED(properties.reported.recipe_campaign.uuid))')}`, { signal: controller.signal })
      .then((response) => setTwins(response.reduce((prev, current) => {
        const { desired } = current.properties;
        const { uuid } = desired.recipe_campaign;
        const obj = { ...prev };
        if (!obj[uuid]) {
          obj[uuid] = [];
        }
        obj[uuid].push(current);
        return obj;
      }, {})))
      .catch((err) => console.error('error fetching digital twins', err));
  }

  function cancelCampaign(device) {
    console.log(device.device_id);
    return authFetch('/api/digital-twins/cancel_campaign', {
      method: 'POST',
      headers: {
        'Accept': 'application/json, text/plain, */*',
        'Content-Type': 'application/json'
      },
      body: JSON.stringify({
        target: device.device_id
      })
    }).then(() => fetchTwins());
  }

  React.useEffect(() => {
    const subscription = authenticationService.currentUser.subscribe((currentUser) => {
      setUser(currentUser);
    });
    return () => {
      subscription.unsubscribe();
    };
  }, []);

  React.useEffect(() => {
    fetchTwins();
    // const response = await (await fetch('/api/digital-twins/query', { headers: authHeader() })).json();
    // setTwins(response);

    return () => {
      controller.abort();
    };
  }, []);

  function renderTwins() {
    if (twins === null) {
      return (
        <Box>
          <Skeleton />
          <Skeleton />
          <Skeleton />
        </Box>
      );
    }
    if (Object.keys(twins).length === 0) {
      return (
        <Box>
          <Typography variant="subtitle">
            <em>There are no pending device updates.</em>
          </Typography>
        </Box>
      );
    }
    const rendered = Object.keys(twins).map((uuid) => {
      const children = twins[uuid];
      const first = children[0];
      const failed = children.filter(isFailedUpdate);

      const rows = children.map((twin) => (
        <TableRow key={twin.device_id}>
          <TableCell>
            <Tooltip title={twin.device_id}><span>{twin.device_id.slice(0, 6)}</span></Tooltip>
          </TableCell>
          <TableCell>{twin.connection_state}</TableCell>
          <TableCell>{isFailedUpdate(twin) ? 'Failed' : 'Pending'}</TableCell>
          <TableCell>
            {twin.tags && twin.tags.address && `${twin.tags.address.city}, ${twin.tags.address.state}`}
          </TableCell>
          <TableCell>
            {twin.tags && twin.tags.geolocation && twin.tags.geolocation.region}
          </TableCell>
          <TableCell>
            <Button color="secondary" size="small" onClick={() => cancelCampaign(twin)}>Cancel</Button>
          </TableCell>
        </TableRow>
      ));

      return (
        <>
          <TableRow>
            <TableCell>{uuid}</TableCell>
            <TableCell>{first.properties.desired.recipe_campaign.filename}</TableCell>
            <TableCell>
              {first.properties.desired.download_window.not_before}
              {' '}
              &mdash;
              {' '}
              {first.properties.desired.download_window.not_after}
            </TableCell>
            <TableCell>
              {`${children.length} pending`}
            </TableCell>
            <TableCell>
              {`${failed.length} failed`}
            </TableCell>
          </TableRow>
          <TableRow>
            <TableCell />
            <TableCell colspan={6}>
              <Table size="small" style={{ border: '1px solid #ddd' }}>
                <TableHead>
                  <TableCell>ID</TableCell>
                  <TableCell>Connection</TableCell>
                  <TableCell>Status</TableCell>
                  <TableCell>Location</TableCell>
                  <TableCell>Region</TableCell>
                  <TableCell />
                </TableHead>
                <TableBody>
                  {rows}
                </TableBody>
              </Table>
            </TableCell>
          </TableRow>
        </>
      );
    });

    return (
      <TableContainer component={Paper} elevation={0}>
        <Table>
          <TableHead>
            <TableRow>
              <TableCell>Campaign</TableCell>
              <TableCell>File</TableCell>
              <TableCell>Time</TableCell>
              <TableCell>Pending</TableCell>
              <TableCell>Failures</TableCell>
            </TableRow>
          </TableHead>
          <TableBody>
            {rendered}
          </TableBody>
        </Table>
      </TableContainer>
    );
  }

  if (!user) {
    return (
      <Box mt={2}>
        <Skeleton />
        <Skeleton />
        <Skeleton />
      </Box>
    );
  }
  return (
    <Box component={Paper} elevation={0} mt={2} px={3} pt={2} pb={4}>
      <Box>
        <Typography variant="h5" gutterBottom>Available Services</Typography>
        <Grid container>
          <Grid item><BigButton to="/upload" icon={<CloudUploadIcon />} text="Upload" /></Grid>
          <Grid item><BigButton to="/devices" icon={<DeviceHubIcon />} text="Devices" /></Grid>
        </Grid>
      </Box>
      <Box mt={4}>
        <Typography variant="h5">Active Campaigns</Typography>
        <Typography variant="caption" gutterBottom>Devices which have not yet downloaded the desired campaign</Typography>
        <Box mt={1}>
          {renderTwins()}
        </Box>
      </Box>
    </Box>
  );
}
