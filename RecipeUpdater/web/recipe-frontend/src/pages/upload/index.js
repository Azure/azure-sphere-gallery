import React from 'react';
import { makeStyles } from '@material-ui/core/styles';
import {
  Box, Paper, Grid, Divider, TextField
} from '@material-ui/core';
import Stepper from '@material-ui/core/Stepper';
import Step from '@material-ui/core/Step';
import StepLabel from '@material-ui/core/StepLabel';
import Button from '@material-ui/core/Button';
import Typography from '@material-ui/core/Typography';
import Table from '@material-ui/core/Table';
import TableBody from '@material-ui/core/TableBody';
import TableCell from '@material-ui/core/TableCell';
import TableContainer from '@material-ui/core/TableContainer';
import TableRow from '@material-ui/core/TableRow';
import DialogTitle from '@material-ui/core/DialogTitle';
import Dialog from '@material-ui/core/Dialog';
import DialogActions from '@material-ui/core/DialogActions';
import DialogContent from '@material-ui/core/DialogContent';
import DialogContentText from '@material-ui/core/DialogContentText';
import { Link as RouterLink } from 'react-router-dom';
import DeviceTwinPicker from '../../components/DeviceTwinPicker';
import authFetch from '../../helpers/auth-fetch';

const useStyles = makeStyles((theme) => ({
  backButton: {
    marginRight: theme.spacing(1)
  },
  instructions: {
    marginTop: theme.spacing(1),
    marginBottom: theme.spacing(1),
    textAlign: 'center'
  }
}));

function getSteps() {
  return ['Upload File', 'Target Devices', 'Create Constraints', 'Deploy'];
}

function getStepContent(stepIndex) {
  const [file, setFile] = React.useState(null);
  const [selectedRows, setSelectedRows] = React.useState([]);
  const [startTime, setStartTime] = React.useState('19:00');
  const [endTime, setEndTime] = React.useState('23:59');
  const [deployStatus, setDeployStatus] = React.useState(null);

  function submit() {
    const body = new FormData();
    setDeployStatus('deploying');

    body.append('file', file);
    body.append('startTime', startTime); // TODO: check fmt
    body.append('endTime', endTime);
    body.append('targets', JSON.stringify(selectedRows));

    authFetch('/api/digital-twins/upload', { method: 'POST', body })
      .then((res) => {
        console.log(res);
        setDeployStatus('done');
      })
      .catch((error) => {
        console.error(error);
        setDeployStatus('error');
      });
  }

  function renderStatus() {
    switch (deployStatus) {
      default:
        return null;
      case 'deploying':
        return (
          <Dialog open>
            <DialogTitle>Deploying...</DialogTitle>
            <DialogContent>
              <DialogContentText>
                Please wait while the file is uploaded to the cloud and digital twins are updated...
              </DialogContentText>
            </DialogContent>
          </Dialog>
        );
      case 'error':
        return (
          <Dialog open>
            <DialogTitle>Error</DialogTitle>
            <DialogContent>
              <DialogContentText>
                An error occured. Please view the console log.
              </DialogContentText>
            </DialogContent>
            <DialogActions>
              <Button variant="contained" color="primary" component={RouterLink} to="/">Return Home</Button>
            </DialogActions>
          </Dialog>
        );
      case 'done':
        return (
          <Dialog open>
            <DialogTitle>Successfully deployed!</DialogTitle>
            <DialogContent>
              <DialogContentText>
                The file has been deployed to Azure and the devices will update shortly.
              </DialogContentText>
            </DialogContent>
            <DialogActions>
              <Button variant="contained" color="primary" component={RouterLink} to="/">Return Home</Button>
            </DialogActions>
          </Dialog>
        );
    }
  }

  switch (stepIndex) {
    case 0:
      if (file) {
        return (
          <Box py={2}>
            <Typography gutterBottom>{file.name}</Typography>
            <Button color="secondary" onClick={() => setFile(null)}>Remove</Button>
          </Box>
        );
      }
      return (
        <Box py={2}>
          <Typography gutterBottom>Select the file to distribute to devices.</Typography>
          <Button
            variant="contained"
            component="label"
          >
            Upload File
            <input
              type="file"
              hidden
              accept=".cbr"
              onChange={(e) => setFile(e.target.files[0])}
            />
          </Button>
        </Box>
      );
    case 1:
      return (
        <Box>
          <Typography>Select the scope of this push.</Typography>
          <DeviceTwinPicker selectedRows={selectedRows} onSelect={(ids) => setSelectedRows(ids)} allowSelection />
        </Box>
      );
    case 2:
      return (
        <Box>
          <Typography gutterBottom>Download Time</Typography>
          <TextField
            required
            label="Start"
            type="time"
            value={startTime}
            onChange={(e) => setStartTime(e.target.value)}
            InputLabelProps={{
              shrink: true
            }}
          />
          &nbsp;
          <TextField
            required
            label="End"
            type="time"
            value={endTime}
            onChange={(e) => setEndTime(e.target.value)}
            InputLabelProps={{
              shrink: true
            }}
          />
        </Box>
      );
    case 3:
      return (
        <Box>
          {renderStatus()}
          <Typography variant="h5" gutterBottom>Summary</Typography>
          <TableContainer>
            <Table>
              <TableBody>
                <TableRow>
                  <TableCell>File</TableCell>
                  <TableCell>{file ? file.name : '—'}</TableCell>
                </TableRow>
                <TableRow>
                  <TableCell>Targets</TableCell>
                  <TableCell>
                    {`${selectedRows.length} selected`}
                  </TableCell>
                </TableRow>
                <TableRow>
                  <TableCell>Download Time</TableCell>
                  <TableCell>
                    {startTime}
                    {' '}
                    &mdash;
                    {' '}
                    {endTime}
                  </TableCell>
                </TableRow>
              </TableBody>
            </Table>
          </TableContainer>
          <Box pt={2}>
            <Button variant="contained" color="primary" size="large" onClick={() => submit()}>Deploy</Button>
          </Box>
        </Box>
      );
    default:
      return 'Unknown stepIndex';
  }
}

export default function UploadPage() {
  const classes = useStyles();
  const [activeStep, setActiveStep] = React.useState(0);
  const steps = getSteps();

  const handleNext = () => {
    setActiveStep((prevActiveStep) => prevActiveStep + 1);
  };

  const handleBack = () => {
    setActiveStep((prevActiveStep) => prevActiveStep - 1);
  };

  const handleReset = () => {
    setActiveStep(0);
  };

  return (
    <Box component={Paper} className={classes.root} elevation={0} mt={2} p={1}>
      <Stepper activeStep={activeStep} alternativeLabel>
        {steps.map((label) => (
          <Step key={label}>
            <StepLabel>{label}</StepLabel>
          </Step>
        ))}
      </Stepper>
      <Box p={2}>
        {activeStep === steps.length ? (
          <>
            <Typography className={classes.instructions}>All steps completed</Typography>
            <Button onClick={handleReset}>Reset</Button>
          </>
        ) : (
          <Grid
            container
            direction="column"
            justify="center"
            alignItems="stretch"
          >
            <Grid item>
              <Typography className={classes.instructions}>{getStepContent(activeStep)}</Typography>
            </Grid>
            <Grid item style={{ textAlign: 'center' }}>
              <Divider style={{ margin: '1em 0' }} />
              <Button
                disabled={activeStep === 0}
                onClick={handleBack}
                className={classes.backButton}
              >
                Back
              </Button>
              {activeStep !== steps.length - 1 && (
              <Button variant="contained" color="primary" onClick={handleNext}>
                Next
              </Button>
              )}
            </Grid>
          </Grid>
        )}
      </Box>
    </Box>
  );
}
