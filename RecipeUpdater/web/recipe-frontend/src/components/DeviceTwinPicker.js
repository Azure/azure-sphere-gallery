/* eslint-disable camelcase */
import React from 'react';
import { Box, Tooltip } from '@material-ui/core';
import { DataGrid } from '@material-ui/data-grid';
import Skeleton from '@material-ui/lab/Skeleton';
import LinearProgress from '@material-ui/core/LinearProgress';

import {
  Query, Builder, Utils as QbUtils
} from 'react-awesome-query-builder';
import MaterialConfig from 'react-awesome-query-builder/lib/config/material';
import 'react-awesome-query-builder/lib/css/styles.css';
import { debounce } from 'debounce';
import authFetch from '../helpers/auth-fetch';
// import { authHeader } from '../../helpers/auth-header';

// import { authenticationService } from '../../helpers/auth-service';
const config = {
  ...MaterialConfig,
  operators: {
    ...MaterialConfig.operators,
    not_is_defined: {
      label: "Is not defined",
      labelForFormat: "NOT IS_DEFINED",
      sqlOp: "NOT IS_DEFINED",
      cardinality: 0,
      reversedOp: "is_defined",
      formatOp: (field) => `NOT IS_DEFINED(${field})`,
      // mongoFormatOp: mongoFormatOp1.bind(null, "$exists", v => false, false),
      jsonLogic: "!"
    },
    is_defined: {
      label: "Is defined",
      labelForFormat: "IS_DEFINED",
      sqlOp: "IS_DEFINED",
      cardinality: 0,
      reversedOp: "not_is_defined",
      formatOp: (field) => `IS_DEFINED(${field})`,
      sqlFormatOp: (field) => `IS_DEFINED(${field})`,
      // mongoFormatOp: mongoFormatOp1.bind(null, "$exists", v => false, false),
      jsonLogic: "!"
    }
  },
  fields: {
    "tags.address.city": {
      label: 'City',
      type: 'text',
      valueSources: ['value'],
      operators: ['equal', 'not_equal', 'is_defined', 'not_is_defined']
    },
    "tags.address.state": {
      label: 'State',
      type: 'text',
      valueSources: ['value'],
      operators: ['equal', 'not_equal', 'is_defined', 'not_is_defined']
    },
    // "tags.address.zip": {
    //   label: 'Zip Code',
    //   type: 'text',
    //   valueSources: ['value'],
    //   operators: ['equal', 'not_equal', 'is_defined', 'not_is_defined']
    // },
    // "tags.geolocation.lat": {
    //   label: 'Latitude',
    //   type: 'number',
    //   valueSources: ['value'],
    //   preferWidgets: ['number'],
    //   fieldSettings: {
    //     min: -90,
    //     max: 90
    //   }
    // },
    // "tags.geolocation.long": {
    //   label: 'Longitude',
    //   type: 'number',
    //   valueSources: ['value'],
    //   preferWidgets: ['number'],
    //   fieldSettings: {
    //     min: -180,
    //     max: 180
    //   }
    // },
    "tags.geolocation.region": {
      label: 'Region',
      type: 'text',
      valueSources: ['value'],
      operators: ['equal', 'not_equal', 'is_defined', 'not_is_defined']
    },
    "tags.store.ownership_model": {
      label: 'Ownership Model',
      type: 'select',
      valueSources: ['value'],
      operators: ['select_equals', 'select_not_equals', 'is_defined', 'not_is_defined'],
      fieldSettings: {
        listValues: [
          { value: 'franchise', title: 'Franchise' },
          { value: 'corporate', title: 'Corporate' }
        ]
      }
    }
  }
};

const columns = [
  { field: 'connection_state', headerName: 'Connection', width: 150 },
  {
    field: 'update_status',
    headerName: 'Status',
    width: 120,
    renderCell: (params) => {
      const { row } = params;
      const { properties } = row;
      const { desired, reported } = properties;

      if (desired && desired.recipe_campaign) {
        if (reported && reported.recipe_campaign) {
          if (reported.recipe_campaign.failed && desired.recipe_campaign.uuid === reported.recipe_campaign.failed.uuid && desired.recipe_campaign.uuid !== reported.recipe_campaign.uuid) {
            return (
              <Tooltip title="This device has a failed update pending. This will continue to retry until a successful update. Deploying a campaign on this device will overwrite the pending campaign." style={{ cursor: 'help' }}>
                <span className="table-cell-trucate">
                  Failed
                  <sup>?</sup>
                </span>
              </Tooltip>
            );
          }
          if (desired.recipe_campaign.uuid !== reported.recipe_campaign.uuid) {
            return (
              <Tooltip title="This device has an update pending. Deploying a campaign on this device will overwrite the pending campaign." style={{ cursor: 'help' }}>
                <span className="table-cell-trucate">
                  Pending
                  <sup>?</sup>
                </span>
              </Tooltip>
            );
          }
        } else {
          return (
            <Tooltip title="This device has an update pending but has not yet connected to Azure IoT Hub. Deploying a campaign on this device will overwrite the pending campaign." style={{ cursor: 'help' }}>
              <span className="table-cell-trucate">
                Pending/Offline
                <sup>?</sup>
              </span>
            </Tooltip>
          );
        }
      }
      return <span className="table-cell-trucate">Ready</span>;
    }
  },
  {
    field: 'address.city',
    headerName: 'City',
    width: 150,
    editable: true,
    valueGetter: (params) => params.row.tags && params.row.tags.address && params.row.tags.address.city
  },
  {
    field: 'address.state',
    headerName: 'State',
    width: 150,
    editable: true,
    valueGetter: (params) => params.row.tags && params.row.tags.address && params.row.tags.address.state
  },
  {
    field: 'store.ownership_model',
    headerName: 'Ownership',
    width: 150,
    editable: true,
    valueGetter: (params) => params.row.tags && params.row.tags.store && params.row.tags.store.ownership_model
  },
  {
    field: 'geolocation.region',
    headerName: 'Region',
    width: 150,
    editable: true,
    valueGetter: (params) => params.row.tags && params.row.tags.geolocation && params.row.tags.geolocation.region
  },
  {
    field: 'device_id',
    headerName: 'ID',
    width: 100,
    renderCell: (params) => {
      const { row } = params;
      const { device_id } = row;
      const truncated = device_id.slice(0, 8);
      return (
        <Tooltip title={device_id}>
          <span className="table-cell-trucate">{truncated}</span>
        </Tooltip>
      );
    }
  }
];
// const queryValue = { id: QbUtils.uuid(), type: "group" };
const queryValue = {
  "id": QbUtils.uuid(),
  "type": "group",
  "properties": { "conjunction": "OR" },
  "children1": {
    [QbUtils.uuid()]: {
      "type": "rule",
      "properties": {
        "field": "tags.address.state", "operator": "is_defined", "value": [], "valueSrc": [], "valueType": []
      }
    }
  }
};

export default function DeviceTwinPicker(props) {
  const [loading, setLoading] = React.useState(true);
  const [state, setState] = React.useState({
    tree: QbUtils.checkTree(QbUtils.loadTree(queryValue), config),
    config
  });
  const [query, setQuery] = React.useState('');
  const { allowSelection, selectedRows, onSelect } = props;
  const [twins, setTwins] = React.useState(null);
  const controller = new AbortController();
  const onChange = debounce((immutableTree, _config) => {
    // Tip: for better performance you can apply `throttle` - see `examples/demo`
    setState({ tree: immutableTree, config: _config });

    console.log('tree', JSON.stringify(QbUtils.getTree(immutableTree)));
    // const jsonTree = QbUtils.getTree(immutableTree);
    console.log(QbUtils.sqlFormat(immutableTree, _config));
    setQuery(QbUtils.sqlFormat(immutableTree, _config) || '');
  }, 100);

  function fetchTwins() {
    setLoading(true);

    return authFetch(`/api/digital-twins/query?where=${encodeURIComponent(query)}`, { signal: controller.signal })
      .then((response) => {
        setTwins(response);
        setLoading(false);
      })
      .catch((err) => console.error('error fetching digital twins', err));
  }

  const renderBuilder = (_props) => (
    <div className="query-builder-container" style={{ padding: "10px" }}>
      <div className="query-builder qb-lite">
        <Builder {..._props} />
      </div>
    </div>
  );

  React.useEffect(() => {
    fetchTwins();
    // const response = await (await fetch('/api/digital-twins/query', { headers: authHeader() })).json();
    // setTwins(response);

    return () => {
      controller.abort();
    };
  }, [query]);

  function handleCellEditCommit(params) {
    const {
      field, value, id, row
    } = params;
    if (row) {
      if (!row.tags) {
        row.tags = {};
      }
      const split = field.split('.');
      if (!row.tags[split[0]]) {
        row.tags[split[0]] = { };
      }
      row.tags[split[0]][split[1]] = value;
    }
    return authFetch('/api/digital-twins/tag', {
      method: 'POST',
      headers: {
        'Accept': 'application/json, text/plain, */*',
        'Content-Type': 'application/json'
      },
      body: JSON.stringify({
        field,
        value,
        target: id
      })
    }).then(() => fetchTwins());
  }
  if (!twins) {
    return (
      <Box>
        <Skeleton />
        <Skeleton />
        <Skeleton />
      </Box>
    );
  }

  const loadingBar = loading ? <LinearProgress /> : null;

  return (
    <Box>
      <Query
        {...config}
        value={state.tree}
        onChange={onChange}
        renderBuilder={renderBuilder}
      />
      {loadingBar}
      <div style={{ height: 400, width: '100%' }}>
        <DataGrid
          rows={twins}
          columns={columns}
          pageSize={10}
          getRowId={(row) => row.device_id}
          checkboxSelection={allowSelection}
          disableSelectionOnClick
          selectionModel={selectedRows}
          onCellEditCommit={handleCellEditCommit}
          onSelectionModelChange={(args) => {
            // const selectedIDs = new Set(args);
            if (onSelect) {
              onSelect(args);
            }
            // const selectedRowData = data.rows.filter((row) =>
            //   selectedIDs.has(row.id)
            // );
            // console.log("selected rowData:", selectedRowData);
          }}
        />
      </div>
    </Box>
  );
}
